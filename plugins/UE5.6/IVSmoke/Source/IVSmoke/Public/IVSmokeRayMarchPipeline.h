// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "SceneTexturesConfig.h"
#include "SceneView.h"
#include "Shader.h"
#include "ShaderCompilerCore.h"
#include "ShaderParameterStruct.h"

// Forward declarations
struct FIVSmokeVolumeGPUData;

//~==============================================================================
// Occupancy System Configuration

/**
 * Occupancy system configuration constants.
 * Tile-based occupancy for efficient empty space skipping.
 *
 * Memory Layout (1080p):
 *   TileCount: 120 × 68 tiles (W/16 × H/16)
 *   StepSlices: 32 (128 steps / 4)
 *   Occupancy Texture: 120 × 68 × 32 = 261,120 texels
 *   Per-texel: uint4 (128 bits for 128 volumes)
 *   Total Memory: ~8.4 MB (View + Light)
 */
struct FIVSmokeOccupancyConfig
{
	/** Tile size in pixels (16×16 = 256 pixels per tile). */
	static constexpr uint32 TileSizeX = 16;
	static constexpr uint32 TileSizeY = 16;

	/** Step divisor for depth slicing (128 steps / 4 = 32 slices). */
	static constexpr uint32 StepDivisor = 4;

	/** Maximum supported volumes (128 = uint4 bitmask). */
	static constexpr uint32 MaxVolumes = 128;

	/** Thread group size for tile setup (64×1 threads for parallel Bitonic Sort). */
	static constexpr uint32 TileSetupThreadsX = 64;
	static constexpr uint32 TileSetupThreadsY = 1;

	/** Thread group size for occupancy build (8×8×4). */
	static constexpr uint32 OccupancyBuildThreadsX = 8;
	static constexpr uint32 OccupancyBuildThreadsY = 8;
	static constexpr uint32 OccupancyBuildThreadsZ = 4;
};

//~==============================================================================
// GPU Data Structures

/**
 * Per-tile metadata computed in Pass 0.
 * Contains depth range and 128-bit volume mask for sparse iteration.
 *
 * Memory: 48 bytes (16-byte aligned, cache-friendly)
 */
struct FIVSmokeTileData
{
	/** Minimum linear depth in tile (near plane). */
	float Near;

	/** Maximum linear depth in tile (far plane, clamped to max ray distance). */
	float Far;

	/** Step size for this tile: (Far - Near) / (StepSliceCount * StepDivisor). */
	float StepSize;

	/**
	 * Total ray-volume intersection length (for early rejection).
	 * Computed using interval merging to correctly handle overlapping volumes.
	 * If zero, no volumes intersect this tile's center ray.
	 */
	float TotalVolumeLength;

	/**
	 * 128-bit volume mask for sparse iteration.
	 * Each bit indicates whether the corresponding volume intersects this tile.
	 * Used by OccupancyBuild for efficient volume iteration.
	 */
	uint32 VolumeMask128[4];

	/**
	 * Maximum distance for light marching from this tile.
	 * Currently unused - light march uses GlobalAABB per-pixel instead.
	 */
	float MaxLightMarchDistance;

	/** Padding for 48-byte alignment. */
	float Padding[3];
};

static_assert(sizeof(FIVSmokeTileData) == 48, "FIVSmokeTileData must be 48 bytes");

//~==============================================================================
// Pass 0: Tile Setup Compute Shader

/**
 * Tile Setup compute shader (Pass 0).
 * Computes per-tile depth range and quick volume mask using wave reduction.
 *
 * Dispatch: (TileCountX, TileCountY, 1)
 * Each thread group processes one tile (8×8 threads, 2×2 pixels each).
 */
class IVSMOKE_API FIVSmokeTileSetupCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = FIVSmokeOccupancyConfig::TileSetupThreadsX;
	static constexpr uint32 ThreadGroupSizeY = FIVSmokeOccupancyConfig::TileSetupThreadsY;
	static constexpr uint32 ThreadGroupSizeZ = 1;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeTileSetupCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeTileSetupCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeTileSetupCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Output: Per-tile data buffer
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FIVSmokeTileData>, TileDataBufferRW)

		// Input: Scene depth for depth range calculation
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTexturesStruct)

		// Volume data for AABB intersection
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeVolumeGPUData>, VolumeDataBuffer)
		SHADER_PARAMETER(uint32, NumActiveVolumes)

		// Tile configuration
		SHADER_PARAMETER(FIntPoint, TileCount)
		SHADER_PARAMETER(uint32, StepSliceCount)
		SHADER_PARAMETER(float, MaxRayDistance)

		// Viewport info
		SHADER_PARAMETER(FIntPoint, ViewportSize)
		SHADER_PARAMETER(FIntPoint, ViewRectMin)

		// Camera parameters for world position reconstruction
		SHADER_PARAMETER(FVector3f, CameraPosition)
		SHADER_PARAMETER(FVector3f, CameraForward)
		SHADER_PARAMETER(FVector3f, CameraRight)
		SHADER_PARAMETER(FVector3f, CameraUp)
		SHADER_PARAMETER(float, TanHalfFOV)
		SHADER_PARAMETER(float, AspectRatio)

		// Depth conversion
		SHADER_PARAMETER(FVector4f, InvDeviceZToWorldZTransform)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_X"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Y"), ThreadGroupSizeY);
		OutEnvironment.SetDefine(TEXT("TILE_SIZE_X"), FIVSmokeOccupancyConfig::TileSizeX);
		OutEnvironment.SetDefine(TEXT("TILE_SIZE_Y"), FIVSmokeOccupancyConfig::TileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_VOLUMES"), FIVSmokeOccupancyConfig::MaxVolumes);
		OutEnvironment.SetDefine(TEXT("STEP_DIVISOR"), FIVSmokeOccupancyConfig::StepDivisor);
		// Enable wave intrinsics (supported on SM5 with DX11.3+)
		OutEnvironment.CompilerFlags.Add(CFLAG_WaveOperations);
	}
};

//~==============================================================================
// Pass 1: Occupancy Build Compute Shader

/**
 * Occupancy Build compute shader (Pass 1).
 * Builds View and Light occupancy 3D textures using tile data.
 *
 * Dispatch: (ceil(TileCountX/8), ceil(TileCountY/8), ceil(StepSliceCount/4))
 * Each texel stores a uint4 bitmask (128 bits for 128 volumes).
 */
class IVSMOKE_API FIVSmokeOccupancyBuildCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = FIVSmokeOccupancyConfig::OccupancyBuildThreadsX;
	static constexpr uint32 ThreadGroupSizeY = FIVSmokeOccupancyConfig::OccupancyBuildThreadsY;
	static constexpr uint32 ThreadGroupSizeZ = FIVSmokeOccupancyConfig::OccupancyBuildThreadsZ;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeOccupancyBuildCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeOccupancyBuildCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeOccupancyBuildCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input: Tile data from Pass 0
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeTileData>, TileDataBuffer)

		// Output: Occupancy 3D textures (uint4 = 128-bit bitmask)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<uint4>, ViewOccupancyRW)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<uint4>, LightOccupancyRW)

		// Volume data
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeVolumeGPUData>, VolumeDataBuffer)
		SHADER_PARAMETER(uint32, NumActiveVolumes)

		// Tile/Occupancy configuration
		SHADER_PARAMETER(FIntPoint, TileCount)
		SHADER_PARAMETER(uint32, StepSliceCount)
		SHADER_PARAMETER(uint32, StepDivisor)

		// Camera parameters for frustum cell calculation
		SHADER_PARAMETER(FVector3f, CameraPosition)
		SHADER_PARAMETER(FVector3f, CameraForward)
		SHADER_PARAMETER(FVector3f, CameraRight)
		SHADER_PARAMETER(FVector3f, CameraUp)
		SHADER_PARAMETER(float, TanHalfFOV)
		SHADER_PARAMETER(float, AspectRatio)

		// Light parameters for light occupancy
		SHADER_PARAMETER(FVector3f, LightDirection)
		SHADER_PARAMETER(float, MaxLightMarchDistance)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_X"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Y"), ThreadGroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Z"), ThreadGroupSizeZ);
		OutEnvironment.SetDefine(TEXT("TILE_SIZE_X"), FIVSmokeOccupancyConfig::TileSizeX);
		OutEnvironment.SetDefine(TEXT("TILE_SIZE_Y"), FIVSmokeOccupancyConfig::TileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_VOLUMES"), FIVSmokeOccupancyConfig::MaxVolumes);
	}
};

//~==============================================================================
// Pass 2: Ray March with Occupancy Compute Shader

/**
 * Multi-Volume Ray March compute shader with Occupancy optimization.
 * Uses precomputed occupancy textures for efficient empty space skipping.
 *
 * Key optimizations:
 * - Slice-level early-out (skip 4 steps at once if empty)
 * - Sparse volume iteration using firstbitlow + bit clear
 * - Light occupancy for light march optimization
 *
 * This is the main multi-volume ray march shader class.
 * Uses Occupancy-based optimization (3-pass pipeline).
 */
class IVSMOKE_API FIVSmokeMultiVolumeRayMarchCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 1;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeMultiVolumeRayMarchCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeMultiVolumeRayMarchCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeMultiVolumeRayMarchCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Output (Dual Render Target)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SmokeAlbedoTex)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SmokeLocalPosAlphaTex)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SmokeWorldPosDepthTex)

		// Occupancy inputs
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeTileData>, TileDataBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<uint4>, ViewOccupancy)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<uint4>, LightOccupancy)

		// Tile configuration
		SHADER_PARAMETER(FIntPoint, TileCount)
		SHADER_PARAMETER(uint32, StepSliceCount)
		SHADER_PARAMETER(uint32, StepDivisor)

		// Input Textures (same as original ray march)
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D, NoiseVolume)
		SHADER_PARAMETER(float, NoiseUVMul)

		// Samplers
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearBorder_Sampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearRepeat_Sampler)

		// Time
		SHADER_PARAMETER(float, ElapsedTime)

		// Viewport
		SHADER_PARAMETER(FIntPoint, TexSize)
		SHADER_PARAMETER(FVector2f, ViewportSize)
		SHADER_PARAMETER(FVector2f, ViewRectMin)

		// Camera
		SHADER_PARAMETER(FVector3f, CameraPosition)
		SHADER_PARAMETER(FVector3f, CameraForward)
		SHADER_PARAMETER(FVector3f, CameraRight)
		SHADER_PARAMETER(FVector3f, CameraUp)
		SHADER_PARAMETER(float, TanHalfFOV)
		SHADER_PARAMETER(float, AspectRatio)

		// Ray Marching Setup
		SHADER_PARAMETER(int32, MaxSteps)
		SHADER_PARAMETER(float, MinStepSize)

		// Multi-Volume Data
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeVolumeGPUData>, VolumeDataBuffer)
		SHADER_PARAMETER(uint32, NumActiveVolumes)

		// Packed Voxel Data
		SHADER_PARAMETER(int, PackedInterval)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D, PackedVoxelAtlas)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D, PackedHoleAtlas)
		SHADER_PARAMETER(FIntVector, VoxelTexSize)
		SHADER_PARAMETER(FIntVector, PackedVoxelTexSize)
		SHADER_PARAMETER(FIntVector, VoxelAtlasCount)
		SHADER_PARAMETER(FIntVector, HoleTexSize)
		SHADER_PARAMETER(FIntVector, PackedHoleTexSize)
		SHADER_PARAMETER(FIntVector, HoleAtlasCount)

		// Scene Textures
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTexturesStruct)
		SHADER_PARAMETER(FVector4f, InvDeviceZToWorldZTransform)

		// Explicit SceneDepth dependency for RDG ordering (Pre-pass mode only)
		// When bUseExplicitSceneDepth=1, shader uses this texture instead of SceneTexturesStruct.SceneDepthTexture
		// This ensures RDG sees the dependency and orders Ray March BEFORE Depth Write
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture_RDGDependency)
		SHADER_PARAMETER(int32, bUseExplicitSceneDepth)

		// View (for BlueNoise access)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

		// Global Smoke Parameters
		SHADER_PARAMETER(float, GlobalAbsorption)
		SHADER_PARAMETER(float, SmokeSize)
		SHADER_PARAMETER(FVector3f, WindDirection)
		SHADER_PARAMETER(float, VolumeRangeOffset)
		SHADER_PARAMETER(float, VolumeEdgeNoiseFadeOffset)
		SHADER_PARAMETER(float, VolumeEdgeFadeSharpness)

		// Rayleigh Scattering
		SHADER_PARAMETER(FVector3f, LightDirection)
		SHADER_PARAMETER(FVector3f, LightColor)
		SHADER_PARAMETER(float, ScatterScale)
		SHADER_PARAMETER(float, ScatteringAnisotropy)

		// Self-Shadowing (Light Marching)
		SHADER_PARAMETER(int32, LightMarchingSteps)
		SHADER_PARAMETER(float, LightMarchingDistance)
		SHADER_PARAMETER(float, LightMarchingExpFactor)
		SHADER_PARAMETER(float, ShadowAmbient)

		// Global AABB for light march distance calculation (per-pixel ray-box intersection)
		SHADER_PARAMETER(FVector3f, GlobalAABBMin)
		SHADER_PARAMETER(FVector3f, GlobalAABBMax)

		// External Shadowing (CSM)
		SHADER_PARAMETER(int32, NumCascades)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, CSMDepthTextureArray)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, CSMVSMTextureArray)
		SHADER_PARAMETER_SAMPLER(SamplerState, CSMSampler)
		SHADER_PARAMETER_ARRAY(FMatrix44f, CSMViewProjectionMatrices, [8])
		SHADER_PARAMETER_SCALAR_ARRAY(float, CSMSplitDistances, [8])
		SHADER_PARAMETER(FVector3f, CSMCameraPosition)
		SHADER_PARAMETER(float, CascadeBlendRange)
		SHADER_PARAMETER_ARRAY(FVector4f, CSMLightCameraPositions, [8])
		SHADER_PARAMETER_ARRAY(FVector4f, CSMLightCameraForwards, [8])

		// VSM parameters
		SHADER_PARAMETER(int32, bEnableVSM)
		SHADER_PARAMETER(float, VSMMinVariance)
		SHADER_PARAMETER(float, VSMLightBleedingReduction)

		// Shadow common parameters
		SHADER_PARAMETER(float, ShadowDepthBias)
		SHADER_PARAMETER(float, ExternalShadowAmbient)

		// Temporal (for TAA integration)
		SHADER_PARAMETER(uint32, FrameNumber)
		SHADER_PARAMETER(float, JitterIntensity)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_X"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Y"), ThreadGroupSizeY);
		OutEnvironment.SetDefine(TEXT("TILE_SIZE_X"), FIVSmokeOccupancyConfig::TileSizeX);
		OutEnvironment.SetDefine(TEXT("TILE_SIZE_Y"), FIVSmokeOccupancyConfig::TileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_VOLUMES"), FIVSmokeOccupancyConfig::MaxVolumes);
		OutEnvironment.SetDefine(TEXT("USE_OCCUPANCY"), 1);
	}
};

//~==============================================================================
// Occupancy Resources Container

/**
 * Container for occupancy resources created per frame.
 * Holds tile data buffer and occupancy textures.
 */
struct FIVSmokeOccupancyResources
{
	/** Per-tile metadata buffer. */
	FRDGBufferRef TileDataBuffer;

	/** View occupancy 3D texture (which volumes are present at each cell). */
	FRDGTextureRef ViewOccupancy;

	/** Light occupancy 3D texture (which volumes affect light at each cell). */
	FRDGTextureRef LightOccupancy;

	/** Tile count (W/16, H/16). */
	FIntPoint TileCount;

	/** Step slice count (MaxSteps / 4). */
	uint32 StepSliceCount;

	FIVSmokeOccupancyResources();
	bool IsValid() const;
};

//~==============================================================================
// Occupancy Renderer Namespace

namespace IVSmokeOccupancy
{
	/**
	 * Compute tile count from viewport size.
	 */
	FIntPoint ComputeTileCount(const FIntPoint& ViewportSize);

	/**
	 * Compute step slice count from max steps.
	 */
	uint32 ComputeStepSliceCount(int32 MaxSteps);

	/**
	 * Create occupancy resources for a frame.
	 */
	FIVSmokeOccupancyResources CreateOccupancyResources(
		FRDGBuilder& GraphBuilder,
		const FIntPoint& TileCount,
		uint32 StepSliceCount);

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
		const FIntPoint& ViewRectMin);

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
		const FIntPoint& ViewportSize);

} // namespace IVSmokeOccupancy
