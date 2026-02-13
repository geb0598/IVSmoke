// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokePostProcessPass.h"
#include "RenderGraphUtils.h"

FRDGTextureRef FIVSmokePostProcessPass::CreateOutputTexture(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef SourceTexture,
	const TCHAR* DebugName,
	EPixelFormat OverrideFormat,
	FIntPoint OverrideExtent,
	ETextureCreateFlags Flags)
{
	FRDGTextureDesc OutputDesc = SourceTexture->Desc;
	OutputDesc.Flags |= Flags;

	// Override format if specified (e.g., PF_FloatRGBA for alpha support)
	if (OverrideFormat != PF_Unknown)
	{
		OutputDesc.Format = OverrideFormat;
	}

	// Override extent if specified (for viewport-sized textures)
	if (OverrideExtent != FIntPoint::ZeroValue)
	{
		OutputDesc.Extent = OverrideExtent;
	}


	FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, DebugName);

	return OutputTexture;
}
