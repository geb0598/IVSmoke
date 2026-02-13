// Copyright (c) 2026, Team SDB. All rights reserved.
#include "MaterialExpressionIVSmoke_TextureSample.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "IVSmokeMaterialExpressions"

UMaterialExpressionIVSmoke_TextureSample::UMaterialExpressionIVSmoke_TextureSample()
{
	TextureType = EIVSmokeTextureType::SmokeColor;
	R = true;
	G = true;
	B = true;
	A = true;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(LOCTEXT("IVSmokeCategory", "IVSmoke"));
#endif

	Outputs.Reset();
	Outputs.Add(FExpressionOutput(TEXT("Color")));
}
#if WITH_EDITOR
int32 UMaterialExpressionIVSmoke_TextureSample::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	if (!Compiler)
	{
		return INDEX_NONE;
	}

	uint32 SceneTextureId = 0;
	switch (TextureType)
	{
	case EIVSmokeTextureType::SmokeColor:
		SceneTextureId = PPI_PostProcessInput0;
		break;
	case EIVSmokeTextureType::SmokeLocalPos:
		SceneTextureId = PPI_PostProcessInput1;
		break;
	case EIVSmokeTextureType::SceneColor:
		SceneTextureId = PPI_PostProcessInput3; // PPI_PostProcessInput3
		break;
	case EIVSmokeTextureType::SmokeWorldPosLinearDepth:
		SceneTextureId = PPI_PostProcessInput4;
		break;
	default:
		return Compiler->Errorf(TEXT("Invalid texture type"));
	}

	int32 UVsInput = INDEX_NONE;

	if (UVs.Expression)
	{
		UVsInput = UVs.Compile(Compiler);
	}
	else
	{
		UVsInput = Compiler->GetViewportUV();

		if (TextureType == EIVSmokeTextureType::SceneColor)
		{
			int32 SceneTexSize = Compiler->GetSceneTextureViewSize(PPI_PostProcessInput3, false); // InvProperty = false
			int32 ViewportSize = Compiler->ViewProperty(MEVP_ViewSize, false);
			int32 NormalizedUV = Compiler->Div(UVsInput, SceneTexSize);
			UVsInput = Compiler->Mul(NormalizedUV, ViewportSize);
		}
	}
	if (UVsInput == INDEX_NONE)
	{
		return Compiler->Errorf(TEXT("Failed to compile UV input"));
	}

	int32 SceneTextureLookup = Compiler->SceneTextureLookup(
		UVsInput,
		SceneTextureId,
		false,
		false,
		false
	);

	if (SceneTextureLookup == INDEX_NONE)
	{
		return Compiler->Errorf(TEXT("Failed to sample scene texture"));
	}

	if (!R && !G && !B && !A)
	{
		return Compiler->Errorf(TEXT("At least one channel must be selected"));
	}

	if (R && G && B && A)
	{
		return SceneTextureLookup;
	}

	return Compiler->ComponentMask(SceneTextureLookup, R, G, B, A);
}

void UMaterialExpressionIVSmoke_TextureSample::GetCaption(TArray<FString>& OutCaptions) const
{
	FString TypeName;
	switch (TextureType)
	{
	case EIVSmokeTextureType::SmokeColor:
		TypeName = TEXT("SmokeColor");
		break;
	case EIVSmokeTextureType::SmokeLocalPos:
		TypeName = TEXT("SmokeLocalPos");
		break;
	case EIVSmokeTextureType::SceneColor:
		TypeName = TEXT("SceneColor");
		break;
	case EIVSmokeTextureType::SmokeWorldPosLinearDepth:
		TypeName = TEXT("SmokeWorldPosLinearDepth");
		break;
	default:
		TypeName = TEXT("Unknown");
		break;
	}
	OutCaptions.Add(FString::Printf(TEXT("IVSmoke Sample [%s]"), *TypeName));
}

uint32 UMaterialExpressionIVSmoke_TextureSample::GetOutputType(int32 OutputIndex)
{
	int32 NumChannels = 0;
	if (R) NumChannels++;
	if (G) NumChannels++;
	if (B) NumChannels++;
	if (A) NumChannels++;

	switch (NumChannels)
	{
	case 1:
		return MCT_Float1;
	case 2:
		return MCT_Float2;
	case 3:
		return MCT_Float3;
	case 4:
	default:
		return MCT_Float4;
	}
}
#endif

#undef LOCTEXT_NAMESPACE
