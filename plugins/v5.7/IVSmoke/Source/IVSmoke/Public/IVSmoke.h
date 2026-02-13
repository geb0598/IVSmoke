// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

inline const FName IVSmokeVoxelVolumeTag = TEXT("IVSmoke.AIVSmokeVoxelVolumeTag");

/** Log category for IVSmoke plugin */
DECLARE_LOG_CATEGORY_EXTERN(LogIVSmoke, Log, All);

DEFINE_LOG_CATEGORY_STATIC(LogIVSmokeVis, Log, All);

DECLARE_STATS_GROUP(TEXT("IVSmoke"), STATGROUP_IVSmoke, STATCAT_Advanced);

//~==============================================================================
// Memory Stats (GPU VRAM)

DECLARE_MEMORY_STAT(TEXT("Noise Volume"), STAT_IVSmoke_NoiseVolume, STATGROUP_IVSmoke);
DECLARE_MEMORY_STAT(TEXT("CSM Shadow Maps"), STAT_IVSmoke_CSMShadowMaps, STATGROUP_IVSmoke);
DECLARE_MEMORY_STAT(TEXT("Per-Frame Textures"), STAT_IVSmoke_PerFrameTextures, STATGROUP_IVSmoke);
DECLARE_MEMORY_STAT(TEXT("Total VRAM"), STAT_IVSmoke_TotalVRAM, STATGROUP_IVSmoke);

class FIVSmokeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
