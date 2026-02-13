// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeSceneViewExtension.h"
#include "IVSmokeRenderer.h"
#include "IVSmokeSettings.h"
#include "IVSmokeShaders.h"
#include "IVSmokeVoxelVolume.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScreenPass.h"
#include "RenderingThread.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "PixelShaderUtils.h"

#if WITH_EDITOR
#include "Editor.h"
#endif
#include "SceneTexturesConfig.h"
#include "SceneRenderTargetParameters.h"

TSharedPtr<FIVSmokeSceneViewExtension, ESPMode::ThreadSafe> FIVSmokeSceneViewExtension::Instance;

FIVSmokeSceneViewExtension::FIVSmokeSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

void FIVSmokeSceneViewExtension::Initialize()
{
	if (!Instance.IsValid())
	{
		Instance = FSceneViewExtensions::NewExtension<FIVSmokeSceneViewExtension>();
	}
}

void FIVSmokeSceneViewExtension::Shutdown()
{
	Instance.Reset();
}

void FIVSmokeSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	// Called ONCE per ViewFamily on Game Thread
	// Per-World architecture: Each World is processed independently
	// Multiple ViewFamilies (Editor, PIE) can coexist without conflict
	FIVSmokeRenderer& Renderer = FIVSmokeRenderer::Get();

	// Get world from ViewFamily
	UWorld* World = nullptr;
	if (InViewFamily.Scene)
	{
		World = InViewFamily.Scene->GetWorld();
	}

	if (!World)
	{
		return;
	}

#if WITH_EDITOR
	// Skip Editor world rendering when PIE is active
	// When playing in editor, user focuses on PIE viewport - no need to render smoke in editor viewports
	// This significantly improves performance by avoiding redundant ray marching in 4 editor viewports
	if (GEditor && GEditor->IsPlayingSessionInEditor())
	{
		if (World->WorldType == EWorldType::Editor)
		{
			// Clear any stale cached render data for Editor world
			ENQUEUE_RENDER_COMMAND(IVSmokeClearEditorRenderData)(
				[&Renderer, World](FRHICommandListImmediate& RHICmdList)
				{
					Renderer.SetCachedRenderData(World, FIVSmokePackedRenderData());
				}
			);
			return;
		}
	}
#endif

	// Sync server time if needed (per-world)
	if (!Renderer.bIsServerTimeSynced(World))
	{
		if (AGameStateBase* GS = World->GetGameState())
		{
			float LocalTime = World->GetTimeSeconds();
			float ServerTime = GS->GetServerWorldTimeSeconds();
			Renderer.SetServerTimeOffset(World, ServerTime - LocalTime);
		}
	}

	// Collect renderable volumes using TActorIterator (Pull-based pattern)
	TArray<AIVSmokeVoxelVolume*> ValidVolumes;
	for (TActorIterator<AIVSmokeVoxelVolume> It(World); It; ++It)
	{
		if (IsValid(*It) && It->ShouldRender())
		{
			ValidVolumes.Add(*It);
		}
	}

	if (ValidVolumes.Num() == 0)
	{
		// Clear cached render data for this World to stop rendering
		ENQUEUE_RENDER_COMMAND(IVSmokeClearRenderData)(
			[&Renderer, World](FRHICommandListImmediate& RHICmdList)
			{
				Renderer.SetCachedRenderData(World, FIVSmokePackedRenderData());
			}
		);
		return;
	}

	// Get camera position from first view for distance-based filtering
	FVector CameraPosition = FVector::ZeroVector;
	if (InViewFamily.Views.Num() > 0 && InViewFamily.Views[0])
	{
		CameraPosition = InViewFamily.Views[0]->ViewLocation;
	}

	// Prepare render data on Game Thread for this World
	// PrepareRenderData includes frame deduplication (skips if already processed this frame)
	FIVSmokePackedRenderData RenderData = Renderer.PrepareRenderData(World, ValidVolumes, CameraPosition);

	// Transfer to Render Thread via command queue
	ENQUEUE_RENDER_COMMAND(IVSmokeSetRenderData)(
		[&Renderer, World, RenderData = MoveTemp(RenderData)](FRHICommandListImmediate& RHICmdList) mutable
		{
			Renderer.SetCachedRenderData(World, MoveTemp(RenderData));
		}
	);
}

bool FIVSmokeSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	// Actual filtering happens in SubscribeToPostProcessingPass where we have access to FSceneView
	return true;
}

void FIVSmokeSceneViewExtension::SubscribeToPostProcessingPass(
	EPostProcessingPass Pass,
	const FSceneView& InView,
	FPostProcessingPassDelegateArray& InOutPassCallbacks,
	bool bIsPassEnabled)
{
	// Skip non-primary views (Scene Capture, Reflection Capture, Planar Reflection, etc.)
	// These views may have invalid projection matrices or extreme FOV values that cause GPU hangs
	if (InView.bIsSceneCapture || InView.bIsReflectionCapture || InView.bIsPlanarReflection)
	{
		return;
	}

	// Always use AfterDOF pass - DOF applied to smoke, best balance of quality and compatibility
	if (Pass == EPostProcessingPass::AfterDOF)
	{
		InOutPassCallbacks.Add(
			FPostProcessingPassDelegate::CreateRaw(
				this,
				&FIVSmokeSceneViewExtension::Render_RenderThread
			)
		);
	}
}

FScreenPassTexture FIVSmokeSceneViewExtension::Render_RenderThread(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FPostProcessMaterialInputs& Inputs)
{
	return FIVSmokeRenderer::Get().Render(GraphBuilder, View, Inputs);
}

void FIVSmokeSceneViewExtension::PostRenderBasePassDeferred_RenderThread(
	FRDGBuilder& GraphBuilder,
	FSceneView& InView,
	const FRenderTargetBindingSlots& RenderTargets,
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	// Skip non-primary views (Scene Capture, Reflection Capture, Planar Reflection, etc.)
	// These views may have invalid projection matrices or extreme FOV values that cause GPU hangs
	if (InView.bIsSceneCapture || InView.bIsReflectionCapture || InView.bIsPlanarReflection)
	{
		return;
	}

	const UIVSmokeSettings* Settings = UIVSmokeSettings::Get();
	if (!Settings)
	{
		return;
	}

	// Pre-pass Pipeline: Ray March → Upscale → UpsampleFilter (→ Depth Write if enabled)
	// Always runs when smoke rendering is enabled.
	// Results are cached in View-based RDG cache for Post-process Visual/Composite passes.
	if (Settings->bEnableSmokeRendering)
	{
		FIVSmokeRenderer::Get().RunPrePassPipeline(GraphBuilder, InView, RenderTargets, SceneTextures);
	}
}

void FIVSmokeSceneViewExtension::PostRenderViewFamily_RenderThread(
	FRDGBuilder& GraphBuilder,
	FSceneViewFamily& InViewFamily)
{
	// Clear per-view data for this specific ViewFamily only
	// Each ViewFamily has its own GraphBuilder, so only clear data belonging to this ViewFamily
	// This prevents race conditions when multiple ViewFamilies render simultaneously (e.g., Editor split viewports)
	FIVSmokeRenderer::Get().ClearViewDataForViewFamily(InViewFamily);
}
