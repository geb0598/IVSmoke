// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeSettings.h"
#include "IVSmokeRenderer.h"
#include "IVSmoke.h"
#include "IVSmokeVisualMaterialPreset.h"
#include "Materials/MaterialInterface.h"


UIVSmokeSettings::UIVSmokeSettings()
{

}

const UIVSmokeSettings* UIVSmokeSettings::Get()
{
	return GetDefault<UIVSmokeSettings>();
}

//~==============================================================================
// Quality Preset Tables
// Index order: Low=0, Medium=1, High=2

namespace IVSmokeQualityPresets
{
	// Ray Marching: MaxSteps, MinStepSize
	static constexpr int32 RayMarchMaxSteps[] = { 128, 256, 512 };
	static constexpr float RayMarchMinStepSize[] = { 50.0f, 25.0f, 16.0f };

	// Self Shadow: LightMarchingSteps (Off=0, Low=1, Medium=2, High=3)
	static constexpr int32 SelfShadowSteps[] = { 0, 3, 6, 8 };

	// External Shadow: NumCascades, Resolution, MaxDistance (Off=0, Low=1, Medium=2, High=3)
	static constexpr int32 ExternalShadowCascades[] = { 0, 3, 4, 4 };
	static constexpr int32 ExternalShadowResolution[] = { 0, 512, 512, 1024 };
	static constexpr float ExternalShadowMaxDistance[] = { 0.0f, 20000.0f, 30000.0f, 50000.0f };

	// Global → External Shadow mapping (Low→Off, Medium→Medium, High→High)
	static constexpr uint8 GlobalToExternalShadow[] = { 0, 2, 3 };  // Low→Off(0), Medium→Medium(2), High→High(3)
}

//~==============================================================================
// Ray Marching Quality Getters

int32 UIVSmokeSettings::GetEffectiveMaxSteps() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		if (RayMarchQuality == EIVSmokeRayMarchQuality::Custom)
		{
			return CustomMaxSteps;
		}
		return IVSmokeQualityPresets::RayMarchMaxSteps[static_cast<uint8>(RayMarchQuality)];
	}
	return IVSmokeQualityPresets::RayMarchMaxSteps[static_cast<uint8>(GlobalQuality)];
}

float UIVSmokeSettings::GetEffectiveMinStepSize() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		if (RayMarchQuality == EIVSmokeRayMarchQuality::Custom)
		{
			return CustomMinStepSize;
		}
		return IVSmokeQualityPresets::RayMarchMinStepSize[static_cast<uint8>(RayMarchQuality)];
	}
	return IVSmokeQualityPresets::RayMarchMinStepSize[static_cast<uint8>(GlobalQuality)];
}

//~==============================================================================
// Self Shadow Quality Getters

bool UIVSmokeSettings::IsSelfShadowingEnabled() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		return SelfShadowQuality != EIVSmokeSelfShadowQuality::Off;
	}
	// Global Low/Medium/High → Self Shadow Low/Medium/High (always enabled)
	return true;
}

int32 UIVSmokeSettings::GetEffectiveLightMarchingSteps() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		if (SelfShadowQuality == EIVSmokeSelfShadowQuality::Custom)
		{
			return CustomLightMarchingSteps;
		}
		return IVSmokeQualityPresets::SelfShadowSteps[static_cast<uint8>(SelfShadowQuality)];
	}
	// Global Low=0, Medium=1, High=2 → Self Shadow Low=1, Medium=2, High=3
	return IVSmokeQualityPresets::SelfShadowSteps[static_cast<uint8>(GlobalQuality) + 1];
}

//~==============================================================================
// External Shadow Quality Getters

bool UIVSmokeSettings::IsExternalShadowingEnabled() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		return ExternalShadowQuality != EIVSmokeExternalShadowQuality::Off;
	}
	// Global Low → External Shadow Off
	return GlobalQuality != EIVSmokeGlobalQuality::Low;
}

int32 UIVSmokeSettings::GetEffectiveNumCascades() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		if (ExternalShadowQuality == EIVSmokeExternalShadowQuality::Custom)
		{
			return CustomNumCascades;
		}
		return IVSmokeQualityPresets::ExternalShadowCascades[static_cast<uint8>(ExternalShadowQuality)];
	}
	uint8 MappedIndex = IVSmokeQualityPresets::GlobalToExternalShadow[static_cast<uint8>(GlobalQuality)];
	return IVSmokeQualityPresets::ExternalShadowCascades[MappedIndex];
}

int32 UIVSmokeSettings::GetEffectiveCascadeResolution() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		if (ExternalShadowQuality == EIVSmokeExternalShadowQuality::Custom)
		{
			return CustomCascadeResolution;
		}
		return IVSmokeQualityPresets::ExternalShadowResolution[static_cast<uint8>(ExternalShadowQuality)];
	}
	uint8 MappedIndex = IVSmokeQualityPresets::GlobalToExternalShadow[static_cast<uint8>(GlobalQuality)];
	return IVSmokeQualityPresets::ExternalShadowResolution[MappedIndex];
}

float UIVSmokeSettings::GetEffectiveShadowMaxDistance() const
{
	if (GlobalQuality == EIVSmokeGlobalQuality::Custom)
	{
		if (ExternalShadowQuality == EIVSmokeExternalShadowQuality::Custom)
		{
			return CustomShadowMaxDistance;
		}
		return IVSmokeQualityPresets::ExternalShadowMaxDistance[static_cast<uint8>(ExternalShadowQuality)];
	}
	uint8 MappedIndex = IVSmokeQualityPresets::GlobalToExternalShadow[static_cast<uint8>(GlobalQuality)];
	return IVSmokeQualityPresets::ExternalShadowMaxDistance[MappedIndex];
}
UIVSmokeVisualMaterialPreset* UIVSmokeSettings::GetVisualMaterialPreset() const
{
	if (CachedVisualMaterialPreset != nullptr)
	{
		return CachedVisualMaterialPreset;
	}
	return nullptr;
}
void UIVSmokeSettings::SetVisualMaterialPreset(FSoftObjectPath VisualMaterialPresetPath)
{
	UIVSmokeSettings* Settings = GetMutableDefault<UIVSmokeSettings>();

	Settings->SmokeVisualMaterialPreset = VisualMaterialPresetPath;
	Settings->CachedVisualMaterialPreset = Cast<UIVSmokeVisualMaterialPreset>(Settings->SmokeVisualMaterialPreset.TryLoad());

	// EnsureIsComplete() requires RHI - skip during commandlets (cooking/packaging)
	if (!IsRunningCommandlet() && CachedVisualMaterialPreset)
	{
		if (UMaterialInterface* Mat = CachedVisualMaterialPreset->SmokeVisualMaterial.Get())
		{
			Mat->EnsureIsComplete();
		}
	}
}

void UIVSmokeSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// Load preset (safe during cooking)
	CachedVisualMaterialPreset = Cast<UIVSmokeVisualMaterialPreset>(SmokeVisualMaterialPreset.TryLoad());

	// EnsureIsComplete() requires RHI - skip during commandlets (cooking/packaging)
	if (!IsRunningCommandlet() && CachedVisualMaterialPreset)
	{
		if (UMaterialInterface* Mat = CachedVisualMaterialPreset->SmokeVisualMaterial.Get())
		{
			Mat->EnsureIsComplete();
		}
	}
}

#if WITH_EDITOR
void UIVSmokeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Global settings are read directly from UIVSmokeSettings::Get() each frame,
	// so no manual refresh is needed when properties change.

	// Load preset and ensure shader compilation when preset changes
	CachedVisualMaterialPreset = Cast<UIVSmokeVisualMaterialPreset>(SmokeVisualMaterialPreset.TryLoad());
	if (CachedVisualMaterialPreset)
	{
		if (UMaterialInterface* Mat = CachedVisualMaterialPreset->SmokeVisualMaterial.Get())
		{
			Mat->EnsureIsComplete();
		}
	}
}
#endif
