// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeRayMarchPipeline.h"
#include "IVSmokeShaders.h"
#include "IVSmoke.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneTexturesConfig.h"
#include "SceneRenderTargetParameters.h"
#include "SystemTextures.h"

//~==============================================================================
// Shader Implementations

IMPLEMENT_GLOBAL_SHADER(FIVSmokeTileSetupCS, "/Plugin/IVSmoke/IVSmokeTileSetupCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FIVSmokeOccupancyBuildCS, "/Plugin/IVSmoke/IVSmokeOccupancyBuildCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FIVSmokeMultiVolumeRayMarchCS, "/Plugin/IVSmoke/IVSmokeMultiVolumeRayMarch.usf", "MainCS", SF_Compute);

//~==============================================================================
// Occupancy Renderer Implementation

namespace IVSmokeOccupancy
{
	/**
	 * Compute tile count from viewport size.
	 */
	FIntPoint ComputeTileCount(const FIntPoint& ViewportSize)
	{
		return FIntPoint(
			FMath::DivideAndRoundUp(ViewportSize.X, (int32)FIVSmokeOccupancyConfig::TileSizeX),
			FMath::DivideAndRoundUp(ViewportSize.Y, (int32)FIVSmokeOccupancyConfig::TileSizeY)
		);
	}

	/**
	 * Compute step slice count from max steps.
	 */
	uint32 ComputeStepSliceCount(int32 MaxSteps)
	{
		return FMath::DivideAndRoundUp((uint32)MaxSteps, FIVSmokeOccupancyConfig::StepDivisor);
	}

	/**
	 * Add Pass 0: Tile Setup.
	 * Computes per-tile depth range and quick volume mask.
	 */
	void AddTileSetupPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGBufferRef VolumeDataBuffer,
		uint32 NumActiveVolumes,
		FRDGBufferRef OutTileDataBuffer,
		const FIntPoint& TileCount,
		uint32 StepSliceCount,
		float MaxRayDistance,
		const FIntPoint& ViewportSize,
		const FIntPoint& ViewRectMin)
	{
		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.FeatureLevel);
		TShaderMapRef<FIVSmokeTileSetupCS> ComputeShader(ShaderMap);

		auto* Parameters = GraphBuilder.AllocParameters<FIVSmokeTileSetupCS::FParameters>();

		// Output
		Parameters->TileDataBufferRW = GraphBuilder.CreateUAV(OutTileDataBuffer);

		// Scene textures
		Parameters->SceneTexturesStruct = GetSceneTextureShaderParameters(View).SceneTextures;

		// Volume data
		Parameters->VolumeDataBuffer = GraphBuilder.CreateSRV(VolumeDataBuffer);
		Parameters->NumActiveVolumes = NumActiveVolumes;

		// Tile configuration
		Parameters->TileCount = TileCount;
		Parameters->StepSliceCount = StepSliceCount;
		Parameters->MaxRayDistance = MaxRayDistance;

		// Viewport
		Parameters->ViewportSize = ViewportSize;
		Parameters->ViewRectMin = ViewRectMin;

		// Camera
		const FViewMatrices& ViewMatrices = View.ViewMatrices;
		Parameters->CameraPosition = FVector3f(ViewMatrices.GetViewOrigin());
		Parameters->CameraForward = FVector3f(View.GetViewDirection());
		Parameters->CameraRight = FVector3f(View.GetViewRight());
		Parameters->CameraUp = FVector3f(View.GetViewUp());

		const FMatrix& ProjMatrix = ViewMatrices.GetProjectionMatrix();
		float ProjM11 = ProjMatrix.M[1][1];

		// Validate projection matrix to prevent inf/NaN
		if (FMath::IsNearlyZero(ProjM11, 1e-6f) || !FMath::IsFinite(ProjM11))
		{
			UE_LOG(LogIVSmoke, Error, TEXT("[TileSetup] Invalid ProjMatrix.M[1][1] = %f (likely orthographic viewport), skipping"), ProjM11);
			return; // Skip rendering for orthographic/invalid views
		}

		Parameters->TanHalfFOV = 1.0f / ProjM11;
		Parameters->AspectRatio = (float)ViewportSize.X / (float)ViewportSize.Y;

		// Depth conversion
		Parameters->InvDeviceZToWorldZTransform = FVector4f(View.InvDeviceZToWorldZTransform);

		// Dispatch: One thread group per tile
		FIntVector GroupCount(TileCount.X, TileCount.Y, 1);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("IVSmoke::TileSetup (%dx%d tiles)", TileCount.X, TileCount.Y),
			ComputeShader,
			Parameters,
			GroupCount
		);
	}

	/**
	 * Add Pass 1: Occupancy Build.
	 * Builds View and Light occupancy 3D textures.
	 */
	void AddOccupancyBuildPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGBufferRef TileDataBuffer,
		FRDGBufferRef VolumeDataBuffer,
		uint32 NumActiveVolumes,
		FRDGTextureRef OutViewOccupancy,
		FRDGTextureRef OutLightOccupancy,
		const FIntPoint& TileCount,
		uint32 StepSliceCount,
		const FVector3f& LightDirection,
		float MaxLightMarchDistance,
		const FIntPoint& ViewportSize)
	{
		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.FeatureLevel);
		TShaderMapRef<FIVSmokeOccupancyBuildCS> ComputeShader(ShaderMap);

		auto* Parameters = GraphBuilder.AllocParameters<FIVSmokeOccupancyBuildCS::FParameters>();

		// Input
		Parameters->TileDataBuffer = GraphBuilder.CreateSRV(TileDataBuffer);
		Parameters->VolumeDataBuffer = GraphBuilder.CreateSRV(VolumeDataBuffer);
		Parameters->NumActiveVolumes = NumActiveVolumes;

		// Output
		Parameters->ViewOccupancyRW = GraphBuilder.CreateUAV(OutViewOccupancy);
		Parameters->LightOccupancyRW = GraphBuilder.CreateUAV(OutLightOccupancy);

		// Configuration
		Parameters->TileCount = TileCount;
		Parameters->StepSliceCount = StepSliceCount;
		Parameters->StepDivisor = FIVSmokeOccupancyConfig::StepDivisor;

		// Camera
		const FViewMatrices& ViewMatrices = View.ViewMatrices;
		Parameters->CameraPosition = FVector3f(ViewMatrices.GetViewOrigin());
		Parameters->CameraForward = FVector3f(View.GetViewDirection());
		Parameters->CameraRight = FVector3f(View.GetViewRight());
		Parameters->CameraUp = FVector3f(View.GetViewUp());

		const FMatrix& ProjMatrix = ViewMatrices.GetProjectionMatrix();
		float ProjM11 = ProjMatrix.M[1][1];

		// Validate projection matrix to prevent inf/NaN
		if (FMath::IsNearlyZero(ProjM11, 1e-6f) || !FMath::IsFinite(ProjM11))
		{
			UE_LOG(LogIVSmoke, Error, TEXT("[OccupancyBuild] Invalid ProjMatrix.M[1][1] = %f (likely orthographic viewport), skipping"), ProjM11);
			return; // Skip rendering for orthographic/invalid views
		}

		Parameters->TanHalfFOV = 1.0f / ProjM11;
		Parameters->AspectRatio = (float)ViewportSize.X / (float)ViewportSize.Y;

		// Light
		Parameters->LightDirection = LightDirection;
		Parameters->MaxLightMarchDistance = MaxLightMarchDistance;

		// Dispatch: ceil(TileCount/8) × ceil(StepSliceCount/4)
		FIntVector GroupCount(
			FMath::DivideAndRoundUp((uint32)TileCount.X, FIVSmokeOccupancyBuildCS::ThreadGroupSizeX),
			FMath::DivideAndRoundUp((uint32)TileCount.Y, FIVSmokeOccupancyBuildCS::ThreadGroupSizeY),
			FMath::DivideAndRoundUp(StepSliceCount, FIVSmokeOccupancyBuildCS::ThreadGroupSizeZ)
		);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("IVSmoke::OccupancyBuild (%dx%dx%d)", TileCount.X, TileCount.Y, StepSliceCount),
			ComputeShader,
			Parameters,
			GroupCount
		);
	}

	/**
	 * Create occupancy resources for a frame.
	 */
	FIVSmokeOccupancyResources CreateOccupancyResources(
		FRDGBuilder& GraphBuilder,
		const FIntPoint& TileCount,
		uint32 StepSliceCount)
	{
		FIVSmokeOccupancyResources Resources;

		// Tile Data Buffer
		const uint32 TileDataCount = TileCount.X * TileCount.Y;
		FRDGBufferDesc TileDataDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FIVSmokeTileData), TileDataCount);
		Resources.TileDataBuffer = GraphBuilder.CreateBuffer(TileDataDesc, TEXT("IVSmoke.TileData"));

		// View Occupancy 3D Texture (uint4 = 128 bits per texel)
		FRDGTextureDesc OccupancyDesc = FRDGTextureDesc::Create3D(
			FIntVector(TileCount.X, TileCount.Y, StepSliceCount),
			PF_R32G32B32A32_UINT,
			FClearValueBinding::Black,
			ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource
		);
		Resources.ViewOccupancy = GraphBuilder.CreateTexture(OccupancyDesc, TEXT("IVSmoke.ViewOccupancy"));
		Resources.LightOccupancy = GraphBuilder.CreateTexture(OccupancyDesc, TEXT("IVSmoke.LightOccupancy"));

		// Store configuration
		Resources.TileCount = TileCount;
		Resources.StepSliceCount = StepSliceCount;

		return Resources;
	}

} // namespace IVSmokeOccupancy

//~==============================================================================
// Occupancy Resources Structure

FIVSmokeOccupancyResources::FIVSmokeOccupancyResources()
	: TileDataBuffer(nullptr)
	, ViewOccupancy(nullptr)
	, LightOccupancy(nullptr)
	, TileCount(0, 0)
	, StepSliceCount(0)
{
}

bool FIVSmokeOccupancyResources::IsValid() const
{
	return TileDataBuffer != nullptr && ViewOccupancy != nullptr && LightOccupancy != nullptr;
}
