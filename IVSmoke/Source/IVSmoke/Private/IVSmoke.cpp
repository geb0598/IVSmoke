// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmoke.h"
#include "IVSmokeSceneViewExtension.h"
#include "IVSmokeSettings.h"
#include "IVSmokeVisualMaterialPreset.h"
#include "IVSmokeVoxelVolume.h"
#include "EngineUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Stats/Stats.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY(LogIVSmoke);

#define LOCTEXT_NAMESPACE "FIVSmokeModule"

void FIVSmokeModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("IVSmoke"))->GetBaseDir(),
		TEXT("Shaders")
	);
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/IVSmoke"), PluginShaderDir);

#if !UE_SERVER
	// SceneViewExtension requires GEngine, defer until engine is ready
	// Skip during commandlets (cooking/packaging) - RHI may not be fully initialized
	if (!IsRunningCommandlet())
	{
		UE_LOG(LogIVSmoke, Log, TEXT("[FIVSmokeModule::StartupModule] Registering OnPostEngineInit"));
		FCoreDelegates::OnPostEngineInit.AddLambda([]()
		{
			UE_LOG(LogIVSmoke, Log, TEXT("[FIVSmokeModule::StartupModule] OnPostEngineInit fired"));

			// Preload Visual Material Preset and ensure shader compilation
			if (const UIVSmokeSettings* Settings = UIVSmokeSettings::Get())
			{
				if (UIVSmokeVisualMaterialPreset* Preset = Settings->GetVisualMaterialPreset())
				{
					if (UMaterialInterface* Mat = Preset->SmokeVisualMaterial.Get())
					{
						Mat->EnsureIsComplete();
						UE_LOG(LogIVSmoke, Log, TEXT("[FIVSmokeModule::StartupModule] Visual Material preloaded"));
					}
				}
			}

			FIVSmokeSceneViewExtension::Initialize();
		});
	}
#endif

#if WITH_EDITOR
	// Lock IVSmokeVoxelVolume actors during PIE to prevent World Partition errors
	FEditorDelegates::PreBeginPIE.AddLambda([](bool bIsSimulating)
	{
		if (GEditor)
		{
			if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
			{
				for (TActorIterator<AIVSmokeVoxelVolume> It(EditorWorld); It; ++It)
				{
					It->SetLockLocation(true);
				}
			}
			// Invalidate cached lock state so HasLockedActors() rechecks
			GEditor->bCheckForLockActors = true;
		}
	});

	FEditorDelegates::EndPIE.AddLambda([](bool bIsSimulating)
	{
		if (GEditor)
		{
			if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
			{
				for (TActorIterator<AIVSmokeVoxelVolume> It(EditorWorld); It; ++It)
				{
					It->SetLockLocation(false);
				}
			}
			// Invalidate cached lock state
			GEditor->bCheckForLockActors = true;
		}
	});
#endif
}

void FIVSmokeModule::ShutdownModule()
{
#if !UE_SERVER
	if (!IsRunningCommandlet())
	{
		FIVSmokeSceneViewExtension::Shutdown();
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FIVSmokeModule, IVSmoke)
