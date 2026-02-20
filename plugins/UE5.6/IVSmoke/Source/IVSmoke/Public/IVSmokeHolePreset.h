// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "IVSmokeHolePreset.generated.h"

/**
 * Type of way the hole is created.
 */
UENUM(BlueprintType)
enum class EIVSmokeHoleType : uint8
{
	/** Fast bullet type. */
	Penetration,

	/** Grenade type. */
	Explosion,

	/** General-purpose mesh type that can be moved. */
	Dynamic,
};

/**
 * Data asset containing hole configuration preset.
 * Automatically registered to global registry on load.
 */
UCLASS(BlueprintType)
class IVSMOKE_API UIVSmokeHolePreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

protected:
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;

public:
	//~============================================================================
	// Common

	/**
	 * Hole Type. 0 = Penetration, 1 = Explosion, 2 = Dynamic
	 * Create a hole in a different way depending on this value.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke")
	EIVSmokeHoleType HoleType = EIVSmokeHoleType::Penetration;

	/**
	 * This radius range that affects.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (ClampMin = "0.1", ClampMax = "1000.0", EditConditionHides, EditCondition = "HoleType != EIVSmokeHoleType::Dynamic"))
	float Radius = 50.0f;

	/**
	 * Total effect duration.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (ClampMin = "0.01", ClampMax = "60.0", Tooltip = "Define how long the hole will last within the smoke"))
	float Duration = 3.0f;

	/**
	 * Softness of the edges.
	 * 0 = hard edge, 1 = soft gradient
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (ClampMin = "0.0", ClampMax = "1.0", Tooltip = "0 = hard edge, 1 = soft gradient"))
	float Softness = 0.3f;

	//~============================================================================
	// Explosion

	/**
	 * Expansion time. The expansion time is used for expansion-related curve values.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Expansion", meta = (ClampMin = "0.0", ClampMax = "60.0", EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Explosion",
		Tooltip = "Expansion time is used for expansion-related curve values."))
	float ExpansionDuration = 0.15f;

	/**
	 * Fade range curve over expansion time.
	 * Use normalized time between 0 and 1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Expansion", meta = (EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Explosion",
		Tooltip = "Fade range curve over expansion time. Use normalized time between 0 and 1"))
	TObjectPtr<UCurveFloat> ExpansionFadeRangeCurveOverTime;

	/**
	 * Fade out range curve over shrink time.
	 * At the end of the expansion time, the shrink time begins.
	 * ShrinkDuration = Duration - ExpansionDuration
	 * Use normalized time between 0 and 1
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Shrink", meta = (EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Explosion",
		Tooltip = "Fade range curve over shrink time. Use normalized time between 0 and 1"))
	TObjectPtr<UCurveFloat> ShrinkFadeRangeCurveOverTime;

	/**
	 * Distortion exp value over expansion time.
	 * 1 - pow((1 - NormalizedTime), ExpValue)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Distortion", meta = (ClampMin = "1.0", ClampMax = "5.0", EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Explosion",
		Tooltip = "Distortion exp value over expansion time. 1 - pow((1 - NormalizedTime), ExpValue)"))
	float DistortionExpOverTime = 1.0f;

	/**
	 * Distortion degree max value.
	 * The degree of distortion is proportional to (dis(hole voxel to explosion point) / radius).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Distortion", meta = (ClampMin = "0.0", ClampMax = "1000.0", EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Explosion",
		Tooltip = "Distortion degree max value."))
	float DistortionDistance = 250.0f;

	//~============================================================================
	// Penetration

	/**
	 * EndRadius represents the radius at the EndPosition in a penetration hole.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (ClampMin = "0.0", ClampMax = "1000.0", EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Penetration",
		Tooltip = "EndRadius represents the radius at the EndPosition in a penetration hole."))
	float EndRadius = 25.0f;

	/**
	 * Bullet thickness for obstacle collision detection.
	 * Larger values make bullets more likely to be blocked by nearby walls.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (ClampMin = "0.1", ClampMax = "50.0", EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Penetration",
		Tooltip = "Bullet thickness for obstacle detection. Larger values make bullets more likely to be blocked by nearby walls."))
	float BulletThickness = 5.0f;

	//~============================================================================
	// Dynamic

	/**
	* The size of a hole
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Dynamic",
		Tooltip = "The size of a hole"))
	FVector3f Extent = FVector3f(50.f, 50.f, 50.f);

	/**
	* Minimum travel distance to make a hole
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke", meta = (ClampMin = "10.0", ClampMax = "500.0", EditConditionHides, EditCondition = "HoleType == EIVSmokeHoleType::Dynamic",
		Tooltip = "Minimum travel distance to make a hole"))
	float DistanceThreshold = 50.0f;

	// ============================================================================

	/** Returns the this preset id. */
	FORCEINLINE uint8 GetPresetID() const { return CachedID; }

	/**
	 * Find and return the preset with the key id.
	 * If not, return nullptr.
	 */
	static TObjectPtr<UIVSmokeHolePreset> FindByID(const uint8 InPresetID);

	/**
	 * Returns the y value corresponding to the x value of the curve.
	 * If the curve is nullptr, return 0.
	 */
	static float GetFloatValue(const TObjectPtr<UCurveFloat> Curve, const float X);

private:
	/** Cached preset id. */
	uint8 CachedID = 0;

	/** Register this preset to global registry. */
	void RegisterToGlobalRegistry();

	/** Unregister this preset from global registry. */
	void UnregisterFromGlobalRegistry();
};
