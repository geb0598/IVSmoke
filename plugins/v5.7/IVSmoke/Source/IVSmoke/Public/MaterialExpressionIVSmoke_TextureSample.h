// Copyright (c) 2026, Team SDB. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionIVSmoke_TextureSample.generated.h"

/**
 * Smoke texture type for IVSmoke_Texture sample node
 */
UENUM(BlueprintType)
enum class EIVSmokeTextureType : uint8
{
	/** PostProcessInput0 (SmokeColor, SmokeAlpha) from upsample filter */
	SmokeColor,
	/** PostProcessInput1 (SmokeLocalPos ) from raymarching */
	SmokeLocalPos,
	/** PostProcessInput3 SceneColor */
	SceneColor,
	/** PostProcessInput4 (SmokeWorldPos, SmokeLinearDepth) from raymarching */
	SmokeWorldPosLinearDepth
};
/**
 *	IVSMoke Dedicated Node Available in Unreal Material
 */
UCLASS(collapsecategories, hidecategories = Object)
class IVSMOKE_API UMaterialExpressionIVSmoke_TextureSample : public UMaterialExpression
{
	GENERATED_BODY()
public:
	UMaterialExpressionIVSmoke_TextureSample();

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual uint32 GetOutputType(int32 OutputIndex) override;
#endif

public:
	/** Features such as input UVs on existing SceneTexture nodes */
	UPROPERTY()
	FExpressionInput UVs;
	/** Textures to be sampled. See EIVSmokeTextureType */
	UPROPERTY(EditAnywhere, Category = "Source")
	EIVSmokeTextureType TextureType;
	/** Output R channel enabled */
	UPROPERTY(EditAnywhere, Category = "MaterialExpression", meta = (ShowAsComponentMask))
	uint32 R : 1;
	/** Output G channel enabled */
	UPROPERTY(EditAnywhere, Category = "MaterialExpression", meta = (ShowAsComponentMask))
	uint32 G : 1;
	/** Output B channel enabled */
	UPROPERTY(EditAnywhere, Category = "MaterialExpression", meta = (ShowAsComponentMask))
	uint32 B : 1;
	/** Output A channel enabled */
	UPROPERTY(EditAnywhere, Category = "MaterialExpression", meta = (ShowAsComponentMask))
	uint32 A : 1;
};
