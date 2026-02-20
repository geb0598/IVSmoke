// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class UIVSmokeHolePreset;
struct FIVSmokeHoleData;

//~============================================================================
// GPU Data Structure

/**
 * @struct FIVSmokeHoleGPU
 * @brief Built from FIVSmokeHoleData + UIVSmokeHolePreset at render time.
 */
struct alignas(16) FIVSmokeHoleGPU
{
	FIVSmokeHoleGPU() = default;

	/**
	 * You can Constructs a FIVSmokeHoleGPU using DynamicHoleData, Preset, and server time.
	 * @param DynamicHoleData		Dynamic hole data.
	 * @param Preset				HolePreset defined as DataAsset.
	 * @param CurrentServerTime		The CurrentServerTime is obtained through the GetSyncedTime function.
	 */
	FIVSmokeHoleGPU(const FIVSmokeHoleData& DynamicHoleData, const UIVSmokeHolePreset& Preset, const float CurrentServerTime);

	//~============================================================================
	// Common

	/** The central point of hole creation. */
	FVector3f Position;

	/** Time after hole is called creation. */
	float CurLifeTime;

	/** 0 = Penetration, 1 = Explosion, 2 = Dynamic */
	int HoleType;

	/** Radius value used to calculate values related to the range. */
	float Radius;

	/** Total duration. */
	float Duration;

	/** Edge smooth range. */
	float Softness;

	//~============================================================================
	// Dynamic

	/** the size of a hole. */
	FVector3f Extent;

	float DynamicPadding;

	//~============================================================================
	// Explosion

	/** Expansion time used only for Explosion. */
	float ExpansionDuration;

	/** Current fadeRange extracted from ExpansionFadeRangeCurveOverTime with values normalized to expansion time. */
	float CurExpansionFadeRangeOverTime;

	/** Current fadeRange extracted from ShrinkFadeRangeCurveOverTime with values normalized to shrink time. */
	float CurShrinkFadeRangeOverTime;

	/** Exponential value of the calculation of the distortion value over expansion time. */
	float DistortionExpOverTime;

	/** Distortion degree max value. */
	float DistortionDistance;

	FVector3f PresetExplosionPadding;

	//~============================================================================
	// Penetration

	/** The point at which the trajectory of the penetration ends. */
	FVector3f EndPosition;

	/** Radius at the end position. */
	float EndRadius;
};

/**
 * @brief Compute shader that carves holes into 3D volume texture.
 */
class IVSMOKE_API FIVSmokeHoleCarveCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 8;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeHoleCarveCS");
	DECLARE_GLOBAL_SHADER(FIVSmokeHoleCarveCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeHoleCarveCS, FGlobalShader);

public:

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Output: 3D Volume Texture (Read and Write) - R16G16B16A16_UNORM channel
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VolumeTexture)

		// Input: Hole data buffer (unified structure)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FIVSmokeHoleGPU>, HoleBuffer)

		// Volume bounds (local space)
		SHADER_PARAMETER(FVector3f, VolumeMin)
		SHADER_PARAMETER(FVector3f, VolumeMax)

		// Volume resolution
		SHADER_PARAMETER(FIntVector, Resolution)

		// Hole parameters
		SHADER_PARAMETER(int32, NumHoles)

		// Noise textures (per HoleType)
		SHADER_PARAMETER_TEXTURE(Texture2D, PenetrationNoiseTexture)
		SHADER_PARAMETER_TEXTURE(Texture2D, ExplosionNoiseTexture)
		SHADER_PARAMETER_TEXTURE(Texture2D, DynamicNoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, NoiseSampler)

		// Noise parameters (per HoleType)
		SHADER_PARAMETER(float, PenetrationNoiseStrength)
		SHADER_PARAMETER(float, PenetrationNoiseScale)
		SHADER_PARAMETER(float, ExplosionNoiseStrength)
		SHADER_PARAMETER(float, ExplosionNoiseScale)
		SHADER_PARAMETER(float, DynamicNoiseStrength)
		SHADER_PARAMETER(float, DynamicNoiseScale)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment
	)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadGroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEZ"), ThreadGroupSizeZ);
	}
};

/**
 * @brief Compute shader for 1D separable blur on 3D volume texture.
 *        Run 3 times (X, Y, Z axis) for full 3D Gaussian blur.
 */
class IVSMOKE_API FIVSmokeHoleBlurCS : public FGlobalShader
{
public:
	static constexpr uint32 ThreadGroupSizeX = 8;
	static constexpr uint32 ThreadGroupSizeY = 8;
	static constexpr uint32 ThreadGroupSizeZ = 8;
	static constexpr const TCHAR* EventName = TEXT("IVSmokeHoleBlurCS");
	DECLARE_GLOBAL_SHADER(FIVSmokeHoleBlurCS);
	SHADER_USE_PARAMETER_STRUCT(FIVSmokeHoleBlurCS, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input: Source volume texture
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<float4>, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)

		// Output: Destination volume texture
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputTexture)

		// Volume resolution
		SHADER_PARAMETER(FIntVector, Resolution)

		// Blur direction: (1,0,0) for X, (0,1,0) for Y, (0,0,1) for Z
		SHADER_PARAMETER(FIntVector, BlurDirection)

		// Blur radius in voxels
		SHADER_PARAMETER(int32, BlurStep)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment
	)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadGroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEZ"), ThreadGroupSizeZ);
	}
};
