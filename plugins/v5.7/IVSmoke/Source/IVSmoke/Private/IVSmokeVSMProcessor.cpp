// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeVSMProcessor.h"
#include "IVSmokeCSMRenderer.h"
#include "IVSmokeShaders.h"
#include "IVSmokePostProcessPass.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"

DEFINE_LOG_CATEGORY_STATIC(LogIVSmokeVSM, Log, All);

// ============================================================================
// Constructor / Destructor
// ============================================================================

FIVSmokeVSMProcessor::FIVSmokeVSMProcessor()
{
}

FIVSmokeVSMProcessor::~FIVSmokeVSMProcessor()
{
}

// ============================================================================
// Process
// ============================================================================

void FIVSmokeVSMProcessor::Process(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef DepthTexture,
	FRDGTextureRef VSMTexture,
	int32 BlurRadius)
{
	if (!DepthTexture || !VSMTexture)
	{
		return;
	}

	// Step 1: Convert depth to variance
	AddDepthToVariancePass(GraphBuilder, DepthTexture, VSMTexture);

	// Step 2: Apply blur if radius > 0
	if (BlurRadius > 0)
	{
		const FIntPoint TextureSize(VSMTexture->Desc.Extent.X, VSMTexture->Desc.Extent.Y);

		// Create temporary texture for ping-pong blur
		FRDGTextureDesc TempDesc = FRDGTextureDesc::Create2D(
			TextureSize,
			PF_G32R32F,  // RG32F for variance
			FClearValueBinding::None,
			TexCreate_ShaderResource | TexCreate_UAV
		);
		FRDGTextureRef TempTexture = GraphBuilder.CreateTexture(TempDesc, TEXT("IVSmokeVSMBlurTemp"));

		// Horizontal blur: VSMTexture -> TempTexture
		AddHorizontalBlurPass(GraphBuilder, VSMTexture, TempTexture, BlurRadius);

		// Vertical blur: TempTexture -> VSMTexture
		AddVerticalBlurPass(GraphBuilder, TempTexture, VSMTexture, BlurRadius);
	}
}

void FIVSmokeVSMProcessor::ProcessCascades(
	FRDGBuilder& GraphBuilder,
	TArray<FIVSmokeCascadeData>& Cascades,
	int32 BlurRadius)
{
	for (int32 CascadeIndex = 0; CascadeIndex < Cascades.Num(); CascadeIndex++)
	{
		FIVSmokeCascadeData& Cascade = Cascades[CascadeIndex];

		if (!Cascade.DepthRT || !Cascade.VSMRT)
		{
			continue;
		}

		// Register external textures (include cascade index for RenderDoc debugging)
		FRDGTextureRef DepthRDG = GraphBuilder.RegisterExternalTexture(
			CreateRenderTarget(
				Cascade.DepthRT->GetRenderTargetResource()->GetRenderTargetTexture(),
				*FString::Printf(TEXT("IVSmokeCSMDepth_%d"), CascadeIndex)
			)
		);

		FRDGTextureRef VSMRDG = GraphBuilder.RegisterExternalTexture(
			CreateRenderTarget(
				Cascade.VSMRT->GetRenderTargetResource()->GetRenderTargetTexture(),
				*FString::Printf(TEXT("IVSmokeCSMVSM_%d"), CascadeIndex)
			)
		);

		Process(GraphBuilder, DepthRDG, VSMRDG, BlurRadius);
	}
}

// ============================================================================
// Depth to Variance Pass
// ============================================================================

void FIVSmokeVSMProcessor::AddDepthToVariancePass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef DepthTexture,
	FRDGTextureRef VSMTexture)
{
	const FIntPoint TextureSize(DepthTexture->Desc.Extent.X, DepthTexture->Desc.Extent.Y);

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FIVSmokeDepthToVarianceCS> ComputeShader(ShaderMap);

	auto* Parameters = GraphBuilder.AllocParameters<FIVSmokeDepthToVarianceCS::FParameters>();
	Parameters->DepthTexture = DepthTexture;
	Parameters->VarianceTexture = GraphBuilder.CreateUAV(VSMTexture);
	Parameters->TextureSize = TextureSize;

	FIVSmokePostProcessPass::AddComputeShaderPass<FIVSmokeDepthToVarianceCS>(
		GraphBuilder,
		ShaderMap,
		ComputeShader,
		Parameters,
		FIntVector(TextureSize.X, TextureSize.Y, 1)
	);
}

// ============================================================================
// Blur Passes
// ============================================================================

void FIVSmokeVSMProcessor::AddHorizontalBlurPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef SourceTexture,
	FRDGTextureRef DestTexture,
	int32 BlurRadius)
{
	const FIntPoint TextureSize(SourceTexture->Desc.Extent.X, SourceTexture->Desc.Extent.Y);

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FIVSmokeVSMBlurCS> ComputeShader(ShaderMap);

	auto* Parameters = GraphBuilder.AllocParameters<FIVSmokeVSMBlurCS::FParameters>();
	Parameters->SourceTexture = SourceTexture;
	Parameters->DestTexture = GraphBuilder.CreateUAV(DestTexture);
	Parameters->LinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Parameters->TextureSize = TextureSize;
	Parameters->BlurRadius = BlurRadius;
	Parameters->BlurDirection = 0;  // Horizontal

	FIVSmokePostProcessPass::AddComputeShaderPass<FIVSmokeVSMBlurCS>(
		GraphBuilder,
		ShaderMap,
		ComputeShader,
		Parameters,
		FIntVector(TextureSize.X, TextureSize.Y, 1)
	);
}

void FIVSmokeVSMProcessor::AddVerticalBlurPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef SourceTexture,
	FRDGTextureRef DestTexture,
	int32 BlurRadius)
{
	const FIntPoint TextureSize(SourceTexture->Desc.Extent.X, SourceTexture->Desc.Extent.Y);

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FIVSmokeVSMBlurCS> ComputeShader(ShaderMap);

	auto* Parameters = GraphBuilder.AllocParameters<FIVSmokeVSMBlurCS::FParameters>();
	Parameters->SourceTexture = SourceTexture;
	Parameters->DestTexture = GraphBuilder.CreateUAV(DestTexture);
	Parameters->LinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Parameters->TextureSize = TextureSize;
	Parameters->BlurRadius = BlurRadius;
	Parameters->BlurDirection = 1;  // Vertical

	FIVSmokePostProcessPass::AddComputeShaderPass<FIVSmokeVSMBlurCS>(
		GraphBuilder,
		ShaderMap,
		ComputeShader,
		Parameters,
		FIntVector(TextureSize.X, TextureSize.Y, 1)
	);
}
