// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IVSmokeSmokePreset.generated.h"

/**
 * Per-volume smoke appearance preset.
 * Controls visual properties that can vary between individual smoke volumes.
 *
 * For global rendering settings (noise, ray marching, scattering, etc.),
 * see UIVSmokeSettings in Project Settings > Plugins > IVSmoke.
 */
UCLASS(BlueprintType)
class IVSMOKE_API UIVSmokeSmokePreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ============================================================================
	// Appearance (Per-Volume)
	// ============================================================================

	/** Base color of the smoke. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke|Appearance")
	FLinearColor SmokeColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);

	/** Light absorption coefficient. Higher = denser, more opaque smoke. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmokeAbsorption = 0.1f;

	/** Base density multiplier for this volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke|Appearance", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float VolumeDensity = 1.0f;

	// ============================================================================
	// UPrimaryDataAsset Interface
	// ============================================================================

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("IVSmokeSmokePreset"), GetFName());
	}
};
