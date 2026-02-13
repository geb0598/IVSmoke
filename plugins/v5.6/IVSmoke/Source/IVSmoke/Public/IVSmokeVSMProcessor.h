// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"

class FRDGBuilder;
struct FIVSmokeCascadeData;

/**
 * Variance Shadow Map processor.
 * Converts depth maps to variance shadow maps and applies blur.
 *
 * VSM stores (depth, depth²) which enables soft shadow filtering
 * without the shadow acne artifacts of PCF.
 */
class IVSMOKE_API FIVSmokeVSMProcessor
{
public:
	FIVSmokeVSMProcessor();
	~FIVSmokeVSMProcessor();

	/**
	 * Process a depth texture into a VSM texture.
	 * Performs depth → variance conversion and separable Gaussian blur.
	 *
	 * @param GraphBuilder RDG builder.
	 * @param DepthTexture Input depth texture (R32F).
	 * @param VSMTexture   Output VSM texture (RG32F).
	 * @param BlurRadius   Blur kernel radius (0 = no blur).
	 */
	void Process(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef DepthTexture,
		FRDGTextureRef VSMTexture,
		int32 BlurRadius
	);

	/**
	 * Process all cascades' depth textures into VSM textures.
	 *
	 * @param GraphBuilder RDG builder.
	 * @param Cascades	 Array of cascade data.
	 * @param BlurRadius   Blur kernel radius.
	 */
	void ProcessCascades(
		FRDGBuilder& GraphBuilder,
		TArray<FIVSmokeCascadeData>& Cascades,
		int32 BlurRadius
	);

private:
	/**
	 * Convert depth to variance (depth, depth²).
	 *
	 * @param GraphBuilder RDG builder.
	 * @param DepthTexture Input depth texture.
	 * @param VSMTexture   Output VSM texture.
	 */
	void AddDepthToVariancePass(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef DepthTexture,
		FRDGTextureRef VSMTexture
	);

	/**
	 * Apply separable Gaussian blur (horizontal pass).
	 *
	 * @param GraphBuilder RDG builder.
	 * @param SourceTexture Input texture.
	 * @param DestTexture   Output texture.
	 * @param BlurRadius	Blur kernel radius.
	 */
	void AddHorizontalBlurPass(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef SourceTexture,
		FRDGTextureRef DestTexture,
		int32 BlurRadius
	);

	/**
	 * Apply separable Gaussian blur (vertical pass).
	 *
	 * @param GraphBuilder RDG builder.
	 * @param SourceTexture Input texture.
	 * @param DestTexture   Output texture.
	 * @param BlurRadius	Blur kernel radius.
	 */
	void AddVerticalBlurPass(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef SourceTexture,
		FRDGTextureRef DestTexture,
		int32 BlurRadius
	);
};
