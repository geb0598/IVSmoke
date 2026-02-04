// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SceneViewExtension.h"
#include "IVSmokeSettings.generated.h"

/**
 * Global quality preset that sets all section quality levels at once.
 */
UENUM(BlueprintType)
enum class EIVSmokeGlobalQuality : uint8
{
	/** All sections set to Low (External Shadow Off). */
	Low UMETA(DisplayName = "Low (Performance)"),

	/** All sections set to Medium. */
	Medium UMETA(DisplayName = "Medium (Balanced)"),

	/** All sections set to High. */
	High UMETA(DisplayName = "High (Quality)"),

	/** Per-section custom configuration. */
	Custom UMETA(DisplayName = "Custom")
};

/**
 * Ray marching quality levels.
 * Controls MaxSteps and MinStepSize for volumetric rendering.
 */
UENUM(BlueprintType)
enum class EIVSmokeRayMarchQuality : uint8
{
	/** Low quality: MaxSteps=128, MinStepSize=50 */
	Low UMETA(DisplayName = "Low"),

	/** Medium quality: MaxSteps=256, MinStepSize=25 */
	Medium UMETA(DisplayName = "Medium"),

	/** High quality: MaxSteps=512, MinStepSize=16 */
	High UMETA(DisplayName = "High"),

	/** User-defined parameters. */
	Custom UMETA(DisplayName = "Custom")
};

/**
 * Self-shadow quality levels.
 * Controls light marching for internal smoke self-shadowing.
 */
UENUM(BlueprintType)
enum class EIVSmokeSelfShadowQuality : uint8
{
	/** Disabled: No self-shadowing. */
	Off UMETA(DisplayName = "Off"),

	/** Low quality: LightMarchingSteps=3 */
	Low UMETA(DisplayName = "Low"),

	/** Medium quality: LightMarchingSteps=6 */
	Medium UMETA(DisplayName = "Medium"),

	/** High quality: LightMarchingSteps=8 */
	High UMETA(DisplayName = "High"),

	/** User-defined parameters. */
	Custom UMETA(DisplayName = "Custom")
};

/**
 * External shadow quality levels.
 * Controls cascaded shadow map settings for external object shadows.
 */
UENUM(BlueprintType)
enum class EIVSmokeExternalShadowQuality : uint8
{
	/** Disabled: No external shadows. */
	Off UMETA(DisplayName = "Off"),

	/** Low quality: NumCascades=3, Resolution=512, MaxDistance=20000 */
	Low UMETA(DisplayName = "Low"),

	/** Medium quality: NumCascades=4, Resolution=512, MaxDistance=30000 */
	Medium UMETA(DisplayName = "Medium"),

	/** High quality: NumCascades=4, Resolution=1024, MaxDistance=50000 */
	High UMETA(DisplayName = "High"),

	/** User-defined parameters. */
	Custom UMETA(DisplayName = "Custom")
};

class UIVSmokeVisualMaterialPreset;
/**
 * Global settings for IVSmoke plugin.
 * Accessible via Project Settings > Plugins > IVSmoke.
 *
 * These settings affect ALL smoke volumes globally.
 * For per-volume appearance (color, density), use UIVSmokeSmokePreset.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "IVSmoke"))
class IVSMOKE_API UIVSmokeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UIVSmokeSettings();

	/** Get the singleton settings instance. */
	static const UIVSmokeSettings* Get();

	//~==============================================================================
	// UDeveloperSettings Interface

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("IVSmoke"); }

	UIVSmokeVisualMaterialPreset* GetVisualMaterialPreset() const;
	virtual void PostInitProperties() override;

	/** Change visual material preset data asset. */
	UFUNCTION(BlueprintCallable, Category = "IVSmoke | VisualMaterial")
	void SetVisualMaterialPreset(FSoftObjectPath VisualMaterialPresetPath);

#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("IVSmoke", "SettingsSection", "IVSmoke"); }
	virtual FText GetSectionDescription() const override { return NSLOCTEXT("IVSmoke", "SettingsDescription", "Configure IVSmoke volumetric smoke settings"); }
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//~==============================================================================
	// General

	/** Enable smoke rendering globally. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | General")
	bool bEnableSmokeRendering = true;

	/** Show advanced options in all categories. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | General")
	bool bShowAdvancedOptions = false;

	/** Global quality preset. Sets all section quality levels at once. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality")
	EIVSmokeGlobalQuality GlobalQuality = EIVSmokeGlobalQuality::Medium;

	/** Ray marching quality level. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom", EditConditionHides))
	EIVSmokeRayMarchQuality RayMarchQuality = EIVSmokeRayMarchQuality::Medium;

	/** Custom: Maximum ray marching steps (32-1024). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (ClampMin = "32", ClampMax = "1024",
			EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom && RayMarchQuality==EIVSmokeRayMarchQuality::Custom",
			EditConditionHides))
	int32 CustomMaxSteps = 256;

	/** Custom: Minimum step size in world units (5-100). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (ClampMin = "5.0", ClampMax = "100.0",
			EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom && RayMarchQuality==EIVSmokeRayMarchQuality::Custom",
			EditConditionHides))
	float CustomMinStepSize = 25.0f;

	/** Self-shadow quality level. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom", EditConditionHides))
	EIVSmokeSelfShadowQuality SelfShadowQuality = EIVSmokeSelfShadowQuality::Medium;

	/** Custom: Number of light marching steps (1-16). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (ClampMin = "1", ClampMax = "16",
			EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom && SelfShadowQuality==EIVSmokeSelfShadowQuality::Custom",
			EditConditionHides))
	int32 CustomLightMarchingSteps = 6;

	/** External shadow quality level. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom", EditConditionHides))
	EIVSmokeExternalShadowQuality ExternalShadowQuality = EIVSmokeExternalShadowQuality::Medium;

	/** Custom: Number of shadow cascades (1-6). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (ClampMin = "1", ClampMax = "6",
			EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom && ExternalShadowQuality==EIVSmokeExternalShadowQuality::Custom",
			EditConditionHides))
	int32 CustomNumCascades = 4;

	/** Custom: Shadow map resolution per cascade (256-2048). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (ClampMin = "256", ClampMax = "2048",
			EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom && ExternalShadowQuality==EIVSmokeExternalShadowQuality::Custom",
			EditConditionHides))
	int32 CustomCascadeResolution = 512;

	/** Custom: Maximum shadow distance in centimeters (1000-100000). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Quality",
		meta = (ClampMin = "1000", ClampMax = "100000",
			EditCondition = "GlobalQuality==EIVSmokeGlobalQuality::Custom && ExternalShadowQuality==EIVSmokeExternalShadowQuality::Custom",
			EditConditionHides))
	float CustomShadowMaxDistance = 50000.0f;

	//~==============================================================================
	// Quality Getters

	/** Get effective MaxSteps based on quality settings. */
	int32 GetEffectiveMaxSteps() const;

	/** Get effective MinStepSize based on quality settings. */
	float GetEffectiveMinStepSize() const;

	/** Check if self-shadowing is enabled based on quality settings. */
	bool IsSelfShadowingEnabled() const;

	/** Get effective light marching steps based on quality settings. */
	int32 GetEffectiveLightMarchingSteps() const;

	/** Check if external shadowing is enabled based on quality settings. */
	bool IsExternalShadowingEnabled() const;

	/** Get effective number of shadow cascades based on quality settings. */
	int32 GetEffectiveNumCascades() const;

	/** Get effective cascade resolution based on quality settings. */
	int32 GetEffectiveCascadeResolution() const;

	/** Get effective shadow max distance based on quality settings. */
	float GetEffectiveShadowMaxDistance() const;

	//~==============================================================================
	// Appearance

	/** Controls edge softness. Lower = softer edges. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmokeDensityFalloff = 0.2f;

	/** Scale for noise sampling. Affects smoke detail size. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Appearance", meta = (ClampMin = "1.0", ClampMax = "1000.0"))
	float SmokeSize = 256.0f;

	/** Wind direction and speed for smoke animation. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Appearance")
	FVector WindDirection = FVector(0.00f, 0.00f, 0.1f);

	/** Volume edge range offset for density falloff. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float VolumeRangeOffset = 0.1;

	/** Noise-based edge fade offset. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Appearance", meta = (ClampMin = "-1.0", ClampMax = "1.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float VolumeEdgeNoiseFadeOffset = 0.1f;

	/** Edge fade sharpness factor. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Appearance", meta = (ClampMin = "0.1", ClampMax = "10.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float VolumeEdgeFadeSharpness = 3.0f;

	//~==============================================================================
	// Lighting

	/** Enable Rayleigh scattering for atmospheric light effects. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting")
	bool bEnableScattering = true;

	/** Scattering intensity multiplier. Higher = more light scattered through smoke. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting", meta = (ClampMin = "0.0", ClampMax = "10.0", EditCondition = "bEnableScattering", EditConditionHides))
	float ScatterScale = 0.5f;

	/** Anisotropy parameter for Henyey-Greenstein phase function.
	 *  0 = isotropic, positive = forward scattering, negative = backward scattering */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting", meta = (ClampMin = "-0.99", ClampMax = "0.99", EditCondition = "bEnableScattering", EditConditionHides))
	float ScatteringAnisotropy = 0.5f;

	/** Override light direction instead of using scene directional light. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting", meta = (EditCondition = "bShowAdvancedOptions", EditConditionHides))
	bool bOverrideLightDirection = false;

	/** Custom light direction (normalized). Used when bOverrideLightDirection is true. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting", meta = (EditCondition = "bShowAdvancedOptions && bOverrideLightDirection", EditConditionHides))
	FVector LightDirectionOverride = FVector(0.0f, 0.0f, 1.0f);

	/** Override light color instead of using scene directional light. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting", meta = (EditCondition = "bShowAdvancedOptions", EditConditionHides))
	bool bOverrideLightColor = false;

	/** Custom light color. Used when bOverrideLightColor is true. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Lighting", meta = (EditCondition = "bShowAdvancedOptions && bOverrideLightColor", EditConditionHides))
	FLinearColor LightColorOverride = FLinearColor::White;

	//~==============================================================================
	// Self-Shadowing (Light Marching)

	/** Minimum brightness in fully shadowed areas (0=dark, 1=no shadow). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | Self", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShadowAmbient = 0.2f;

	/** Maximum distance to march toward light (0=no limit). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | Self", meta = (ClampMin = "0.0", ClampMax = "500.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float LightMarchingDistance = 0.0f;

	/** Exponential distribution factor for light marching steps (1=uniform, 2-3=recommended). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | Self", meta = (ClampMin = "1.0", ClampMax = "5.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float LightMarchingExpFactor = 2.0f;

	//~==============================================================================
	// External Shadows (Scene Capture)

	/** Minimum brightness in externally shadowed areas (0=dark, 1=no shadow). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExternalShadowAmbient = 0.3f;

	/** Enable Variance Shadow Maps for soft shadows. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External")
	bool bEnableVSM = true;

	/** VSM blur kernel radius (0=no blur). Higher = softer shadows. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0", ClampMax = "8", EditCondition = "bEnableVSM", EditConditionHides))
	int32 VSMBlurRadius = 2;

	/** Shadow depth bias to prevent shadow acne. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0.0", ClampMax = "100.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float ShadowDepthBias = 1.0f;

	/** Include skeletal meshes (characters) in shadow capture. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (EditCondition = "bShowAdvancedOptions", EditConditionHides))
	bool bCaptureSkeletalMeshes = false;

	/** Log/Linear cascade split blend (0=linear, 1=logarithmic). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float CascadeLogLinearBlend = 0.85f;

	/** Blend region at cascade boundaries (0-0.3). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0.0", ClampMax = "0.3", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float CascadeBlendRange = 0.1f;

	/** Minimum variance for VSM to prevent artifacts. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0.01", ClampMax = "100.0", EditCondition = "bShowAdvancedOptions && bEnableVSM", EditConditionHides))
	float VSMMinVariance = 1.0f;

	/** VSM light bleeding reduction (0=none). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shadows | External", meta = (ClampMin = "0.0", ClampMax = "0.5", EditCondition = "bShowAdvancedOptions && bEnableVSM", EditConditionHides))
	float VSMLightBleedingReduction = 0.2f;

	// TODO: Priority Update system disabled - causes shadow flickering due to texel snapping
	// synchronization issues. Properties kept for serialization compatibility.
	// See IVSmokeCSMRenderer::UpdateCascadePriorities for details.

	UPROPERTY()
	bool bEnablePriorityUpdate = false;

	UPROPERTY()
	int32 NearCascadeUpdateInterval = 1;

	UPROPERTY()
	int32 FarCascadeUpdateInterval = 4;

	//~==============================================================================
	// Post Processing (Voxel FXAA)

	/** FXAA maximum edge search distance. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | PostProcessing", meta = (ClampMin = "0.0", ClampMax = "4.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float FXAASpanMax = 4.0f;

	/** FXAA edge detection threshold range. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | PostProcessing", meta = (ClampMin = "0.0", ClampMax = "8.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float FXAARange = 1.2f;

	/** FXAA sharpness factor. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | PostProcessing", meta = (ClampMin = "0.1", ClampMax = "8.0", EditCondition = "bShowAdvancedOptions", EditConditionHides))
	float FXAASharpness = 1.7f;

	//~==============================================================================
	// Rendering

	/** Smoke visual material data asset. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering")
	FSoftObjectPath SmokeVisualMaterialPreset;

	/** Write smoke depth to scene depth buffer for correct translucent sorting.
	 *  When enabled, particles behind opaque smoke regions are correctly occluded. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering",
		meta = (EditCondition = "bShowAdvancedOptions", EditConditionHides))
	bool bEnableDepthWrite = true;

	/** Depth bias in centimeters. Positive values push depth further from camera. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering",
		meta = (ClampMin = "0.0", ClampMax = "100.0", EditCondition = "bShowAdvancedOptions && bEnableDepthWrite", EditConditionHides))
	float DepthWriteBias = 50.0f;

	//~==============================================================================
	// Debug

	/** Show debug visualization for smoke volumes. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Debug")
	bool bShowDebugVolumes = false;


private:

	/** Cached smoke visual material preset. */
	UIVSmokeVisualMaterialPreset* CachedVisualMaterialPreset;
};
