// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RHI.h"
#include "RHIStaticStates.h"
#include "RenderGraphUtils.h"
#include "SceneTexturesConfig.h"
#include "SceneView.h"
#include "ShaderParameterStruct.h"

//~==============================================================================
// GPU Data Structures for Multi-Volume Rendering

/**
 * GPU-side volume metadata for single-pass multi-volume ray marching.
 * Each volume has its own transform, bounds, and rendering parameters.
 * This struct is uploaded to a StructuredBuffer for GPU access.
 *
 * Memory layout: 256 bytes (aligned to 16-byte boundary)
 */
struct FIVSmokeVolumeGPUData
{
	/** Grid resolution (voxel count per axis). */
	FIntVector3 GridResolution;     // 12 bytes
	/** Total voxel count for this volume. */
	uint32 VoxelCount;              // 4 bytes

	/** Smoke color for this volume. */
	FVector3f SmokeColor;           // 12 bytes
	/** Absorption coefficient. */
	float Absorption;               // 4 bytes

	/** Center offset for grid-to-local coordinate conversion. */
	FVector3f CenterOffset;         // 12 bytes
	/** Per-volume density multiplier (default 1.0). */
	float DensityScale;             // 4 bytes

	/** World-space AABB minimum (for fast ray-box intersection). */
	FVector3f VolumeWorldAABBMin;         // 12 bytes
	float VoxelSize;                // 4 bytes

	/** World-space AABB maximum (for fast ray-box intersection). */
	FVector3f VolumeWorldAABBMax;         // 12 bytes
	uint32 VoxelBufferOffset;       // 4 bytes

	FVector3f VoxelWorldAABBMin;	// 12 bytes
	float FadeInDuration;			// 4 bytes
	FVector3f VoxelWorldAABBMax;	// 12 bytes
	float FadeOutDuration;			// 4 bytes

	float Reserved[4];              // 16 bytes (future use / alignment)
};

// Ensure structure is 256 bytes for efficient GPU access
static_assert(sizeof(FIVSmokeVolumeGPUData) % 16 == 0, "FIVSmokeVolumeGPUData size error");

// Note: FIVSmokeMultiVolumeRayMarchCS is now defined in IVSmokeOccupancy.h
// (Occupancy-based ray marching has replaced the original implementation)

class IVSMOKE_API FIVSmokeNoiseGeneratorGlobalCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 8;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeNoiseGeneratorGlobalCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeNoiseGeneratorGlobalCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeNoiseGeneratorGlobalCS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Output noise volume texture. */
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, RWNoiseTex)
		/** Texture resolution (voxel count per axis). */
		SHADER_PARAMETER(FUintVector3, TexSize)
		/** Number of fractal octaves for noise generation. */
		SHADER_PARAMETER(int32, Octaves)
		/** Wrap value for seamless tiling. */
		SHADER_PARAMETER(float, Wrap)
		/** Number of cells per axis for cellular noise. */
		SHADER_PARAMETER(int32, AxisCellCount)
		/** Noise amplitude (intensity). */
		SHADER_PARAMETER(float, Amplitude)
		/** Size of each noise cell. */
		SHADER_PARAMETER(int32, CellSize)
		/** Random seed for noise generation. */
		SHADER_PARAMETER(int32, Seed)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
class IVSMOKE_API FIVSmokeStructuredToTextureCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 8;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeStructuredToTextureCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeStructuredToTextureCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeStructuredToTextureCS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Output 3D texture atlas containing voxel density values. */
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, Desti)
		/** Per-voxel birth times for fade-in animation. */
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, BirthTimes)
		/** Per-voxel death times for fade-out animation. */
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, DeathTimes)
		/** Per-volume GPU metadata (transform, bounds, etc.). */
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeVolumeGPUData>, VolumeDataBuffer)

		/** Total atlas texture size. */
		SHADER_PARAMETER(FIntVector, TexSize)
		/** Voxel resolution per volume. */
		SHADER_PARAMETER(FIntVector, VoxelResolution)
		/** Spacing between volumes in atlas. */
		SHADER_PARAMETER(int32, PackedInterval)
		/** Number of volumes per axis in atlas (3D grid layout). */
		SHADER_PARAMETER(FIntVector, VoxelAtlasCount)
		/** Current game time for fade animation calculation. */
		SHADER_PARAMETER(float, GameTime)
		/** Number of active volumes (for bounds checking). */
		SHADER_PARAMETER(int32, VolumeCount)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
class IVSMOKE_API FIVSmokeVoxelFXAACS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 8;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeVoxelFXAACS");

	DECLARE_GLOBAL_SHADER(FIVSmokeVoxelFXAACS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeVoxelFXAACS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Output anti-aliased 3D texture. */
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, Desti)
		/** Source 3D texture to apply FXAA. */
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<float>, Source)
		/** Linear sampler with border addressing. */
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearBorder_Sampler)
		/** Texture resolution. */
		SHADER_PARAMETER(FIntVector, TexSize)

		/** Maximum edge span for FXAA. */
		SHADER_PARAMETER(float, FXAASpanMax)
		/** Edge detection range threshold. */
		SHADER_PARAMETER(float, FXAARange)
		/** Sharpness factor for anti-aliasing. */
		SHADER_PARAMETER(float, FXAASharpness)
	END_SHADER_PARAMETER_STRUCT()
};

class IVSMOKE_API FIVSmokeCompositePS : public FGlobalShader
{
public:
	static constexpr const TCHAR* EventName = TEXT("IVSmokeCompositePS");
	static FRHIBlendState* GetBlendState()
	{
		return TStaticBlendState<>::GetRHI();
	}

	DECLARE_GLOBAL_SHADER(FIVSmokeCompositePS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeCompositePS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Scene color texture (background). */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneTex)
		/** Smoke albedo (color) from ray marching. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SmokeTex)
		/** Linear sampler with repeat addressing. */
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClamp_Sampler)
		/** Viewport size for UV calculation. */
		SHADER_PARAMETER(FVector2f, ViewportSize)
		/** View rect offset for multi-view support. */
		SHADER_PARAMETER(FVector2f, ViewRectMin)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
class IVSMOKE_API FIVSmokeCopyPS : public FGlobalShader
{
public:
	static constexpr const TCHAR* EventName = TEXT("IVSmokeCopyPS");
	static FRHIBlendState* GetBlendState()
	{
		return TStaticBlendState<>::GetRHI();
	}

	DECLARE_GLOBAL_SHADER(FIVSmokeCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeCopyPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Source texture to copy. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MainTex)
		/** Linear sampler for bilinear filtering. */
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearRepeat_Sampler)
		/** Destination texture size for UV mapping. */
		SHADER_PARAMETER(FVector2f, ViewportSize)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class IVSMOKE_API FIVSmokeUpsampleFilterPS : public FGlobalShader
{
public:
	static constexpr const TCHAR* EventName = TEXT("IVSmokeUpsampleFilterPS");
	static FRHIBlendState* GetBlendState()
	{
		return TStaticBlendState<>::GetRHI();
	}

	DECLARE_GLOBAL_SHADER(FIVSmokeUpsampleFilterPS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeUpsampleFilterPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Scene color texture. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneTex)
		/** Smoke albedo (color) from ray marching. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SmokeAlbedoTex)
		/** Smoke opacity mask from ray marching. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SmokeLocalPosAlphaTex)
		/** Linear sampler with repeat addressing. */
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClamp_Sampler)	
		/** UpSample Filter Type. (0 = None, 1 = Sharpen, 2 = Blur, 3 = Median) */
		SHADER_PARAMETER(int, UpSampleFilterType)
		/** Sharpeness filter strength. */
		SHADER_PARAMETER(float, SharpenStrength)
		/** 3x3 gaussian blur strength. */
		SHADER_PARAMETER(float, BlurStrength)
		/** Viewport size for UV calculation. */
		SHADER_PARAMETER(FVector2f, ViewportSize)
		/** View rect offset for multi-view support. */
		SHADER_PARAMETER(FVector2f, ViewRectMin)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

//~==============================================================================
// VSM (Variance Shadow Map) Shaders

/**
 * Depth to Variance compute shader.
 * Converts depth texture (R32F) to variance texture (RG32F).
 * Output: (depth, depth²)
 */
class IVSMOKE_API FIVSmokeDepthToVarianceCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 1;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeDepthToVarianceCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeDepthToVarianceCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeDepthToVarianceCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Input depth texture from shadow capture. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTexture)
		/** Output variance texture (depth, depth squared). */
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, VarianceTexture)
		/** Texture resolution. */
		SHADER_PARAMETER(FIntPoint, TextureSize)
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
	}
};

/**
 * VSM Gaussian blur compute shader.
 * Performs separable Gaussian blur on variance texture.
 * Uses horizontal or vertical direction based on BlurDirection parameter.
 */
class IVSMOKE_API FIVSmokeVSMBlurCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 1;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeVSMBlurCS");

	DECLARE_GLOBAL_SHADER(FIVSmokeVSMBlurCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeVSMBlurCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Source variance texture to blur. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
		/** Output blurred variance texture. */
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, DestTexture)
		/** Linear sampler with clamp addressing. */
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		/** Texture resolution. */
		SHADER_PARAMETER(FIntPoint, TextureSize)
		/** Blur kernel radius in pixels. */
		SHADER_PARAMETER(int32, BlurRadius)
		/** Blur direction (0 = Horizontal, 1 = Vertical). */
		SHADER_PARAMETER(int32, BlurDirection)
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
	}
};

//~==============================================================================
// Depth Write Shaders

/**
 * Pre-Pass Depth Write pixel shader.
 * Writes smoke depth for correct translucent sorting.
 * Only writes depth for nearly opaque pixels (Alpha >= 0.99).
 */
class IVSMOKE_API FIVSmokeDepthWritePS : public FGlobalShader
{
public:
	static constexpr const TCHAR* EventName = TEXT("IVSmokeDepthWritePS");

	DECLARE_GLOBAL_SHADER(FIVSmokeDepthWritePS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeDepthWritePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/** Smoke world position + depth from ray march. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SmokeWorldPosDepthTex)
		/** Smoke local position + alpha from ray march. */
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SmokeLocalPosAlphaTex)
		/** Linear sampler with clamp addressing. */
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		/** Camera forward direction. */
		SHADER_PARAMETER(FVector3f, CameraForward)
		/** Camera world position. */
		SHADER_PARAMETER(FVector3f, CameraOrigin)
		/** Viewport size for UV calculation. */
		SHADER_PARAMETER(FVector2f, ViewportSize)
		/** ViewRect minimum offset. */
		SHADER_PARAMETER(FVector2f, ViewRectMin)
		/** Depth bias in centimeters. */
		SHADER_PARAMETER(float, DepthBias)
		/** Projection matrix element [2][2] for depth conversion. */
		SHADER_PARAMETER(float, ViewToClip_22)
		/** Projection matrix element [3][2] for depth conversion. */
		SHADER_PARAMETER(float, ViewToClip_32)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
