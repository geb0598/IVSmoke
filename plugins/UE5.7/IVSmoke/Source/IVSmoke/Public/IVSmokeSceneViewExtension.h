// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "RenderGraphDefinitions.h"
#include "SceneTexturesConfig.h"

struct FPostProcessMaterialInputs;
struct FScreenPassTexture;
struct FRenderTargetBindingSlots;

/**
 * Scene view extension for IVSmoke post-process hook.
 * Delegates actual rendering to FIVSmokeRenderer.
 */
class IVSMOKE_API FIVSmokeSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FIVSmokeSceneViewExtension(const FAutoRegister& AutoRegister);

	/** Initialize the scene view extension singleton. */
	static void Initialize();

	/** Shutdown and release the scene view extension. */
	static void Shutdown();

	//~ Begin FSceneViewExtensionBase Interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SubscribeToPostProcessingPass(
		EPostProcessingPass Pass,
		const FSceneView& InView,
		FPostProcessingPassDelegateArray& InOutPassCallbacks,
		bool bIsPassEnabled) override;
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;
	virtual void PostRenderBasePassDeferred_RenderThread(
		FRDGBuilder& GraphBuilder,
		FSceneView& InView,
		const FRenderTargetBindingSlots& RenderTargets,
		TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	virtual void PostRenderViewFamily_RenderThread(
		FRDGBuilder& GraphBuilder,
		FSceneViewFamily& InViewFamily) override;
	//~ End FSceneViewExtensionBase Interface

private:
	/** Singleton instance. */
	static TSharedPtr<FIVSmokeSceneViewExtension, ESPMode::ThreadSafe> Instance;

	/** Main render callback for post-process pass. Delegates to FIVSmokeRenderer. */
	FScreenPassTexture Render_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs);
};
