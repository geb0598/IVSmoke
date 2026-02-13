// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IVSmokeVisualMaterialPreset.generated.h"

class UMaterialInterface;

/**
 * It is filter type that's applied after raymarching
 */
UENUM(BlueprintType)
enum class EIVSmokeUpSampleFilterType : uint8
{
	/** Not used filter */
	None,
	/** Sharpen filter */
	Sharpen,
	/** Gaussian blur filter */
	Blur,
	/** Median filter */
	Median
};

/**
 * Data asset containing visual material and alpha process configuration preset.
 */
UCLASS(BlueprintType)
class IVSMOKE_API UIVSmokeVisualMaterialPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()
protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:
	/** It is used in Visual Pass, which is called after upsample filter pass */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering")
	TObjectPtr<UMaterialInterface> SmokeVisualMaterial;

	/** It is filter type that's applied after raymarching */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering")
	EIVSmokeUpSampleFilterType UpSampleFilterType = EIVSmokeUpSampleFilterType::Blur;

	/** The strength of the Sharpen filter.  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering", meta = (ClampMin = "0.0", ClampMax = "1.0",
		EditCondition = "UpSampleFilterType == EIVSmokeUpSampleFilterType::Sharpen", EditConditionHides))
	float SharpenStrength = 0.4f;

	/** The strength of the Gaussian blur filter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IVSmoke | Rendering", meta = (ClampMin = "0.0", ClampMax = "1.0",
		EditCondition = "UpSampleFilterType == EIVSmokeUpSampleFilterType::Blur", EditConditionHides))
	float BlurStrength = 0.4f;
};
