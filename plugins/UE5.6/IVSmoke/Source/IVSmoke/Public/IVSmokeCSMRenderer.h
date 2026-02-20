// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTarget2D.h"

class USceneCaptureComponent2D;
class AActor;
class UWorld;

//~==============================================================================
// Cascade Data Structure

/**
 * Data for a single shadow cascade.
 * Contains render targets, matrices, and update state.
 *
 * TIMING MODEL (Synchronous Capture):
 * ============================================================================
 * We use manual CaptureScene() calls instead of bCaptureEveryFrame to ensure
 * VP matrix and depth texture are always synchronized within the same frame.
 *
 * Frame N timeline:
 *   1. Game Thread: Update() calculates VP_N and calls CaptureScene()
 *   2. Render Thread: Pre-pass ray march uses VP_N with Depth_N ✓
 *   3. Render Thread: Post-process composite
 * ============================================================================
 */
struct IVSMOKE_API FIVSmokeCascadeData
{
	/** Cascade index (0 = nearest, N-1 = farthest). */
	int32 CascadeIndex = 0;

	/** View distance range for this cascade. */
	float NearPlane = 0.0f;
	float FarPlane = 0.0f;

	//~==============================================================================
	// Current Frame Values (used for both capture and shader sampling)

	/** Orthographic projection width for this cascade. */
	float OrthoWidth = 0.0f;

	/** View-projection matrix for world-to-light-clip transform. */
	FMatrix ViewProjectionMatrix = FMatrix::Identity;

	/** Light camera position for this cascade (after texel snapping). */
	FVector LightCameraPosition = FVector::ZeroVector;

	/** Light camera forward direction (light travel direction, opposite of light source). */
	FVector LightCameraForward = FVector(0.0f, 0.0f, -1.0f);

	//~==============================================================================
	// Resources

	/** Depth render target (R32F). */
	TObjectPtr<UTextureRenderTarget2D> DepthRT = nullptr;

	/** Variance Shadow Map render target (RG32F). */
	TObjectPtr<UTextureRenderTarget2D> VSMRT = nullptr;

	/** Scene capture component for this cascade. */
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent = nullptr;

	/** Whether this cascade needs capture this frame. */
	bool bNeedsCapture = true;

	/** Frame number when last captured. */
	uint32 LastCaptureFrame = 0;
};

/**
 * GPU-side cascade data for shader access.
 * Packed for efficient GPU transfer.
 */
struct IVSMOKE_API FIVSmokeCSMGPUData
{
	/** View-projection matrix for this cascade. */
	FMatrix44f ViewProjectionMatrix;

	/** Split distance (far plane of this cascade). */
	float SplitDistance;

	/** Padding for 16-byte alignment. */
	float Padding[3];
};

static_assert(sizeof(FIVSmokeCSMGPUData) == 80, "FIVSmokeCSMGPUData size must be 80 bytes");

//~==============================================================================
// CSM Renderer

/**
 * Cascaded Shadow Map renderer for volumetric smoke.
 * Manages multiple shadow cascades with priority-based updates.
 *
 * Features:
 * - Configurable cascade count (1-8)
 * - Log/Linear split distribution
 * - Texel snapping for shimmer prevention
 * - Priority-based update (near cascades update more frequently)
 * - VSM support for soft shadows
 */
class IVSMOKE_API FIVSmokeCSMRenderer
{
public:
	FIVSmokeCSMRenderer();
	~FIVSmokeCSMRenderer();

	//~==============================================================================
	// Lifecycle

	/**
	 * Initialize CSM renderer with specified settings.
	 * Creates cascade render targets and capture components.
	 *
	 * @param World		  World to create capture components in.
	 * @param NumCascades Number of shadow cascades (1-8).
	 * @param Resolution  Shadow map resolution per cascade.
	 * @param MaxDistance Maximum shadow distance in world units.
	 */
	void Initialize(UWorld* World, int32 NumCascades, int32 Resolution, float MaxDistance);

	/** Release all resources. */
	void Shutdown();

	/** Check if renderer is initialized. */
	bool IsInitialized() const { return bIsInitialized; }

	//~==============================================================================
	// Update

	/**
	 * Update shadow cascades for current frame.
	 * Calculates cascade splits, applies texel snapping, and triggers captures.
	 *
	 * @param CameraPosition Current camera world position.
	 * @param CameraForward  Camera forward direction.
	 * @param LightDirection Direction TOWARD the light source (opposite of light travel).
	 * @param FrameNumber	Current frame number for priority update.
	 */
	void Update(
		const FVector& CameraPosition,
		const FVector& CameraForward,
		const FVector& LightDirection,
		uint32 FrameNumber
	);

	//~==============================================================================
	// Accessors

	/** Get number of active cascades. */
	int32 GetNumCascades() const { return Cascades.Num(); }

	/** Get cascade data by index. */
	const FIVSmokeCascadeData& GetCascade(int32 Index) const { return Cascades[Index]; }

	/** Get all cascade data. */
	const TArray<FIVSmokeCascadeData>& GetCascades() const { return Cascades; }

	/** Get cascade split distances (for shader). */
	TArray<float> GetSplitDistances() const;

	/** Get VSM texture for a cascade (nullptr if VSM disabled). */
	FTextureRHIRef GetVSMTexture(int32 CascadeIndex) const;

	/** Get depth texture for a cascade. */
	FTextureRHIRef GetDepthTexture(int32 CascadeIndex) const;

	/** Check if any cascade has valid shadow data. */
	bool HasValidShadowData() const;

	/**
	 * Get light camera position for a cascade.
	 *
	 * @param CascadeIndex Index of cascade.
	 * @return Light camera world position for the specified cascade.
	 */
	FVector GetLightCameraPosition(int32 CascadeIndex) const;

	/** Get main camera position. */
	FVector GetMainCameraPosition() const { return MainCameraPosition; }

private:
	//~==============================================================================
	// Internal Methods

	/**
	 * Calculate cascade split distances using log/linear blend.
	 *
	 * @param NearPlane	   Near plane distance.
	 * @param FarPlane	   Far plane distance (max shadow distance).
	 * @param LogLinearBlend Blend factor (0=linear, 1=logarithmic).
	 */
	void CalculateCascadeSplits(float NearPlane, float FarPlane, float LogLinearBlend);

	/**
	 * Apply texel snapping to prevent shadow shimmer.
	 *
	 * @param LightViewOrigin Original light view origin.
	 * @param OrthoWidth	  Orthographic projection width.
	 * @param Resolution	  Shadow map resolution.
	 * @param LightRight	  Light's right vector.
	 * @param LightUp		 Light's up vector.
	 * @return Snapped position.
	 */
	FVector ApplyTexelSnapping(
		const FVector& LightViewOrigin,
		float OrthoWidth,
		int32 Resolution,
		const FVector& LightRight,
		const FVector& LightUp
	) const;

	/**
	 * Determine which cascades need update based on priority.
	 *
	 * @param FrameNumber Current frame number.
	 */
	void UpdateCascadePriorities(uint32 FrameNumber);

	/**
	 * Update a single cascade's capture settings.
	 *
	 * @param CascadeIndex   Index of cascade to update.
	 * @param CameraPosition Camera world position.
	 * @param LightDirection Light direction (toward light).
	 */
	void UpdateCascadeCapture(
		int32 CascadeIndex,
		const FVector& CameraPosition,
		const FVector& LightDirection
	);

	/**
	 * Calculate view-projection matrix for a cascade.
	 * Uses current cascade values (LightCameraPosition, LightCameraForward, OrthoWidth)
	 * and stores result in ViewProjectionMatrix.
	 *
	 * @param Cascade Cascade data to update.
	 */
	void CalculateViewProjectionMatrix(FIVSmokeCascadeData& Cascade);

	/**
	 * Create render targets for a cascade.
	 *
	 * @param Cascade   Cascade data to initialize.
	 * @param Resolution Shadow map resolution.
	 */
	void CreateCascadeRenderTargets(FIVSmokeCascadeData& Cascade, int32 Resolution);

	/**
	 * Configure scene capture component settings.
	 *
	 * @param CaptureComponent Component to configure.
	 */
	void ConfigureCaptureComponent(USceneCaptureComponent2D* CaptureComponent);

	//~==============================================================================
	// Member Variables

	/** All cascade data. */
	TArray<FIVSmokeCascadeData> Cascades;

	/** Owner actor for capture components. */
	TWeakObjectPtr<AActor> CaptureOwner;

	/** Current shadow map resolution. */
	int32 CurrentResolution = 512;

	/** Maximum shadow distance. */
	float MaxShadowDistance = 100000.0f;

	/** Log/Linear blend factor for cascade splits. */
	float LogLinearBlend = 0.7f;

	/** Near plane for cascade 0. */
	float NearPlaneDistance = 10.0f;

	/** Initialization state. */
	bool bIsInitialized = false;

	/** Enable priority-based updates. */
	bool bEnablePriorityUpdate = true;

	/** Near cascade update interval (frames). */
	int32 NearCascadeUpdateInterval = 1;

	/** Far cascade update interval (frames). */
	int32 FarCascadeUpdateInterval = 4;

	/** Main camera position (stored for camera-relative calculations). */
	FVector MainCameraPosition = FVector::ZeroVector;
};
