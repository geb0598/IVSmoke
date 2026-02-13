// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"
#include "IVSmoke.h"
#include "IVSmokeShaders.h"
#include "SceneView.h"
#include "IVSmokeSettings.h"
#include "IVSmokeVisualMaterialPreset.h"

class AIVSmokeVoxelVolume;
class FRDGBuilder;
class FSceneView;
class UIVSmokeSmokePreset;
class UTextureRenderTargetVolume;
struct FPostProcessMaterialInputs;
class FIVSmokeCSMRenderer;
class FIVSmokeVSMProcessor;
struct FIVSmokeOccupancyResources;

//~==============================================================================
// Internal Noise Generation Constants

/**
 * Internal noise generation configuration.
 * These values are tuned for optimal smoke appearance and should not be exposed to users.
 */
struct FIVSmokeNoiseConfig
{
	static constexpr int32 Seed = 0;
	static constexpr int32 TexSize = 128;
	static constexpr int32 Octaves = 6;
	static constexpr float Wrap = 0.76f;
	static constexpr float Amplitude = 0.62f;
	static constexpr int32 AxisCellCount = 4;
	static constexpr int32 CellSize = 32;

	/** UV multiplier for noise sampling. Fixed at 1.0; use SmokeSize to control detail frequency. */
	static constexpr float NoiseUVMul = 1.0f;
};

//~==============================================================================
// Render Data Structures (Thread-Safe Data Transfer)

/**
 * Packed render data for all smoke volumes.
 * Created on Game Thread, consumed on Render Thread.
 * Contains all data needed for rendering without accessing Volume actors.
 *
 * THREAD SAFETY:
 * - Created on Game Thread in PrepareRenderData()
 * - Transferred to Render Thread via ENQUEUE_RENDER_COMMAND
 * - Consumed on Render Thread in Render() and RunPrePassPipeline()
 *
 * DATA SAFETY WARNING:
 * Some fields are REFERENCE-BASED and tied to UObject lifetime.
 * See comments below for dangerous vs safe fields.
 */
struct IVSMOKE_API FIVSmokePackedRenderData
{
	//~==========================================================================
	// Safe Fields (Value-Copied)
	// These are independent of source UObject lifetime.

	/** Packed voxel birth times for all volumes (flattened by VoxelBufferOffset). */
	TArray<float> PackedVoxelBirthTimes;

	/** Packed voxel death times for all volumes (flattened by VoxelBufferOffset). */
	TArray<float> PackedVoxelDeathTimes;

	/** Per-volume GPU metadata */
	TArray<FIVSmokeVolumeGPUData> VolumeDataArray;

	/** Common resolution info */
	FIntVector VoxelResolution = FIntVector::ZeroValue;
	FIntVector HoleResolution = FIntVector::ZeroValue;
	int32 VolumeCount = 0;

	/** Preset parameters (copied from default preset) */
	int32 MaxSteps = 128;
	float GlobalAbsorption = 0.1f;
	float SmokeSize = 128.0f;
	float SmokeDensityFalloff = 0.2f;
	FVector WindDirection = FVector(0.01f, 0.02f, 0.1f);
	float VolumeRangeOffset = 0.1f;
	float VolumeEdgeNoiseFadeOffset = 0.04f;
	float VolumeEdgeFadeSharpness = 3.5f;

	/** Scattering parameters */
	bool bEnableScattering = true;
	float ScatterScale = 0.5f;
	float ScatteringAnisotropy = 0.5f;
	FVector LightDirection = FVector(0.2f, 0.1f, 0.9f);
	FLinearColor LightColor = FLinearColor::White;
	float LightIntensity = 1.0f;

	/** Self-shadowing parameters */
	bool bEnableSelfShadowing = true;
	int32 LightMarchingSteps = 6;
	float LightMarchingDistance = 0.0f;
	float LightMarchingExpFactor = 2.0f;
	float ShadowAmbient = 0.2f;

	/** External shadowing parameters (CSM - Cascaded Shadow Maps) */
	/** Note: CSM is always used when external shadowing is enabled */
	int32 NumCascades = 0;
	TArray<FMatrix> CSMViewProjectionMatrices;
	TArray<float> CSMSplitDistances;
	TArray<FVector> CSMLightCameraPositions;
	TArray<FVector> CSMLightCameraForwards;
	float CascadeBlendRange = 0.1f;
	float ShadowDepthBias = 1.0f;
	float ExternalShadowAmbient = 0.3f;

	/** VSM parameters */
	bool bEnableVSM = true;
	float VSMMinVariance = 0.0001f;
	float VSMLightBleedingReduction = 0.2f;

	/** Main camera position for CSM (must match what CSMRenderer used) */
	FVector CSMMainCameraPosition = FVector::ZeroVector;

	/** Validity flag */
	bool bIsValid = false;

	/** Game World Time */
	float GameTime = 0.0f;

	/** Rendering Info (safe - copied values) */
	int UpSampleFilterType = 2;
	float SharpenStrength = 0.0f;
	float BlurStrength = 0.4f;

	//~==========================================================================
	// DANGEROUS: Reference-Based Fields
	// WARNING: These fields hold references to resources owned by UObjects.
	// They become INVALID when the owning UObject is destroyed.
	// ALWAYS call FlushRenderingCommands() before destroying source UObjects.

	/** Hole Texture references - owned by UIVSmokeHoleGeneratorComponent */
	TArray<FTextureRHIRef> HoleTextures;
	TArray<FIntVector> HoleTextureSizes;

	/** CSM Texture references - owned by FIVSmokeCSMRenderer (per-world) */
	TArray<FTextureRHIRef> CSMDepthTextures;
	TArray<FTextureRHIRef> CSMVSMTextures;

	/** Material reference - owned by UIVSmokeVisualMaterialPreset asset */
	UMaterialInterface* SmokeVisualMaterial = nullptr;

	//~==========================================================================

	/** Reset to invalid state */
	void Reset()
	{
		PackedVoxelBirthTimes.Empty();
		PackedVoxelDeathTimes.Empty();
		VolumeDataArray.Empty();
		HoleTextures.Empty();
		HoleTextureSizes.Empty();
		VolumeCount = 0;
		bIsValid = false;

		// CSM
		NumCascades = 0;
		CSMDepthTextures.Empty();
		CSMVSMTextures.Empty();
		CSMViewProjectionMatrices.Empty();
		CSMSplitDistances.Empty();
		CSMLightCameraPositions.Empty();
		CSMLightCameraForwards.Empty();

		SmokeVisualMaterial = nullptr;
	}
};

//~==============================================================================
// Per-World Data Structure

/**
 * Per-World rendering data and resources.
 * Each UWorld (Editor, PIE, Standalone) has its own instance.
 *
 * This enables simultaneous rendering of multiple worlds without data conflicts.
 * Owned by FIVSmokeRenderer::WorldDataMap.
 *
 * LIFECYCLE:
 * - Created on first render request for a World (lazy initialization)
 * - Destroyed when World ends (via OnWorldBeginTearDown delegate)
 *
 * THREAD SAFETY:
 * - Access must be protected by FIVSmokeRenderer::WorldDataMutex
 */
struct IVSMOKE_API FPerWorldData
{
	FPerWorldData();
	~FPerWorldData();  // Defined in cpp - required for TUniquePtr with forward-declared types

	// Non-copyable, non-movable (managed via TSharedPtr)
	FPerWorldData(const FPerWorldData&) = delete;
	FPerWorldData& operator=(const FPerWorldData&) = delete;
	FPerWorldData(FPerWorldData&&) = delete;
	FPerWorldData& operator=(FPerWorldData&&) = delete;

	/** Cached render data for Render Thread. Set via SetCachedRenderData from ENQUEUE_RENDER_COMMAND. */
	FIVSmokePackedRenderData CachedRenderData;

	/** Cached render data for Game Thread. Used for frame deduplication when multiple ViewFamilies exist. */
	FIVSmokePackedRenderData GameThreadCachedRenderData;

	/** CSM renderer for external shadowing (per-world). */
	TUniquePtr<FIVSmokeCSMRenderer> CSMRenderer;

	/** VSM processor for variance shadow mapping (per-world). */
	TUniquePtr<FIVSmokeVSMProcessor> VSMProcessor;

	/** Frame number when CSM was last updated (prevents multiple updates per frame). */
	uint32 LastCSMUpdateFrameNumber = 0;

	/** Frame number when VSM was last processed (prevents duplicate processing per view). */
	uint32 LastVSMProcessFrameNumber = 0;

	/** True if server time offset has been initialized for this World. */
	bool bServerTimeSynced = false;

	/** Server time offset for animation sync. (ServerTime = LocalTime + Offset) */
	float ServerTimeOffset = 0.f;

	/** Frame number when PrepareRenderData was last called (prevents duplicate calls per frame). */
	uint32 LastPreparedFrameNumber = 0;

	/** Reset all data to initial state. Does NOT cleanup CSM (call CleanupCSM separately). */
	void Reset()
	{
		CachedRenderData.Reset();
		LastCSMUpdateFrameNumber = 0;
		LastVSMProcessFrameNumber = 0;
		bServerTimeSynced = false;
		ServerTimeOffset = 0.f;
		LastPreparedFrameNumber = 0;
	}

	/** Cleanup CSM resources. Must be called before destruction. Defined in cpp file. */
	void CleanupCSM();
};

//~==============================================================================
// Renderer

/**
 * Manages registered smoke volumes and handles rendering.
 * Owns shared rendering resources (noise volume) and reads settings from UIVSmokeSettings.
 *
 * ARCHITECTURE: Per-World Data
 * - Each UWorld has its own FPerWorldData instance stored in WorldDataMap
 * - Enables simultaneous rendering of Editor and PIE worlds
 * - World lifecycle managed via OnWorldBeginTearDown delegate
 *
 * See Docs/IVSmoke_RendererArchitecture.md for detailed documentation.
 */
class IVSMOKE_API FIVSmokeRenderer
{
public:
	static FIVSmokeRenderer& Get();

	//~==============================================================================
	// Lifecycle

	/** Initialize renderer resources. Called on first use or settings change. */
	void Initialize();

	/** Release renderer resources. */
	void Shutdown();

	/** Check if renderer is initialized with valid resources. */
	bool IsInitialized() const { return NoiseVolume != nullptr; }

	//~==============================================================================
	// Per-World Data Management

	/**
	 * Get or create per-world data for a specific World.
	 * Creates new FPerWorldData if not exists and registers cleanup delegate.
	 * Must be called on Game Thread.
	 *
	 * @param World The world to get data for
	 * @return Shared pointer to world data (never null for valid World)
	 */
	TSharedPtr<FPerWorldData> GetOrCreateWorldData(UWorld* World);

	/**
	 * Get per-world data for a specific World (read-only).
	 * Returns nullptr if World is not registered.
	 * Thread-safe (uses mutex internally).
	 *
	 * @param World The world to get data for
	 * @return Shared pointer to world data, or nullptr if not found
	 */
	TSharedPtr<FPerWorldData> GetWorldData(UWorld* World);

	/**
	 * Cleanup and remove per-world data.
	 * Called automatically when World is destroyed (via delegate).
	 * IMPORTANT: Calls FlushRenderingCommands() to ensure GPU safety.
	 *
	 * @param World The world to cleanup
	 */
	void CleanupWorldData(UWorld* World);

	/** Check if server time offset was set for a World. */
	bool bIsServerTimeSynced(UWorld* World);

	/** Set server time offset for smoke wind animation for a specific World. */
	void SetServerTimeOffset(UWorld* World, float InServerTimeOffset);

	//~==============================================================================
	// Thread-Safe Render Data (Game Thread <-> Render Thread)

	/** Maximum number of volumes supported for rendering. */
	static constexpr int32 MaxSupportedVolumes = 128;

	/**
	 * Prepare render data from all registered volumes for a specific World.
	 * Must be called on Game Thread.
	 * Copies and packs all volume data for safe Render Thread access.
	 * If volume count exceeds MaxSupportedVolumes (128), filters by distance from camera.
	 * Skips processing if already called this frame (uses LastPreparedFrameNumber).
	 *
	 * @param World The world these volumes belong to
	 * @param InVolumes Array of volumes to process
	 * @param CameraPosition Camera world position for distance-based filtering
	 * @return Packed render data ready for Render Thread
	 */
	FIVSmokePackedRenderData PrepareRenderData(UWorld* World, const TArray<AIVSmokeVoxelVolume*>& InVolumes, const FVector& CameraPosition);

	/**
	 * Set cached render data for a specific World.
	 * Called from Render Thread via ENQUEUE_RENDER_COMMAND.
	 *
	 * @param World The world to set data for
	 * @param InRenderData The render data to cache
	 */
	void SetCachedRenderData(UWorld* World, FIVSmokePackedRenderData&& InRenderData);

	//~==============================================================================
	// Rendering

	/**
	 * Main render entry point called from SceneViewExtension.
	 *
	 * @param GraphBuilder    RDG builder
	 * @param View            Current scene view
	 * @param Inputs          Post-process material inputs
	 * @return Output texture after smoke rendering
	 */
	FScreenPassTexture Render(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs
	);

	//~==============================================================================
	// Pre-Pass Pipeline (Public API for SceneViewExtension)

	/**
	 * Execute Pre-pass pipeline: Ray March → Upscale → UpsampleFilter → Depth Write.
	 * Called from PostRenderBasePassDeferred_RenderThread BEFORE Translucent Pass.
	 * Results are cached for Post-process Visual/Composite passes.
	 *
	 * Pipeline order ensures Ray March reads only opaque depth (before smoke depth write).
	 *
	 * @param GraphBuilder       RDG builder
	 * @param View               Current scene view
	 * @param RenderTargets      Render target slots (for depth write)
	 * @param SceneTextures      Scene texture uniform buffer (for depth sampling in ray march)
	 */
	void RunPrePassPipeline(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const struct FRenderTargetBindingSlots& RenderTargets,
		TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures
	);

private:
	FIVSmokeRenderer();   // Defined in cpp for TUniquePtr with forward-declared types
	~FIVSmokeRenderer();

	FIntVector GetAtlasTexCount(const FIntVector& TexSize, const int32 TexCount, const int32 TexturePackInterval, const int32 TexturePackMaxSize) const;
	//~==============================================================================
	// Resource Management

	/** Create noise volume texture using settings from UIVSmokeSettings. */
	void CreateNoiseVolume();

	/** Get the effective preset for a volume (override or default). */
	const UIVSmokeSmokePreset* GetEffectivePreset(const AIVSmokeVoxelVolume* Volume) const;

	//~==============================================================================
	// Pass Functions

	/**
	 * Multi-Volume Ray Marching with Occupancy Optimization (Three-Pass Pipeline).
	 * Uses tile-based occupancy grid for efficient empty space skipping.
	 * Processes all volumes with correct Beer-Lambert integration.
	 * Outputs to Dual Render Targets (Albedo + Mask) at reduced resolution.
	 *
	 * Pipeline:
	 *   Pass 0: Tile Setup - Compute per-tile depth range and quick volume mask
	 *   Pass 1: Occupancy Build - Build View and Light occupancy 3D textures
	 *   Pass 2: Ray March - Use occupancy for efficient skipping
	 *
	 * @param GraphBuilder       RDG builder
	 * @param View               Current scene view
	 * @param RenderData         Pre-packed render data (created on Game Thread)
	 * @param SmokeAlbedoTex     UAV texture for smoke color output
	 * @param SmokeMaskTex       UAV texture for smoke opacity mask
	 * @param TexSize            Size of output textures (may be reduced resolution)
	 * @param ViewportSize       Size of the full viewport for depth sampling
	 * @param ViewRectMin        Offset into full scene texture for depth sampling
	 */
	/**
	 * @param SceneDepthForDependency  Optional: Pass SceneDepth texture to create explicit RDG dependency.
	 *                                 In Pre-pass, this ensures Ray March reads SceneDepth BEFORE Depth Write modifies it.
	 *                                 Use RenderTargets.DepthStencil.GetTexture() when calling from Pre-pass.
	 */
	void AddMultiVolumeRayMarchPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FIVSmokePackedRenderData& RenderData,
		FRDGTextureRef SmokeAlbedoTex,
		FRDGTextureRef SmokeLocalPosAlpha,
		FRDGTextureRef SmokeWorldPosDepth,
		const FIntPoint& TexSize,
		const FIntPoint& ViewportSize,
		const FIntPoint& ViewRectMin,
		FRDGTextureRef SceneDepthForDependency = nullptr
	);

	/**
	 * Copy/Resize Pass using bilinear sampling.
	 * Used for upscaling (1/2 resolution to Full) to improve quality.
	 *
	 * @param GraphBuilder       RDG builder
	 * @param View               Current scene view
	 * @param SourceTex          Source texture to copy from
	 * @param DestSize           Destination texture size (resizes via bilinear)
	 * @param TexName            Debug name for the created texture
	 * @return                   New texture at DestSize with copied content
	 */
	FRDGTextureRef AddCopyPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGTextureRef SourceTex,
		const FIntPoint& DestSize,
		const TCHAR* TexName
	);

	/**
	 * Copy Pass to existing texture.
	 * Copies SourceTex to DestTex using bilinear sampling.
	 *
	 * @param GraphBuilder       RDG builder
	 * @param View               Current scene view
	 * @param SourceTex          Source texture to copy from
	 * @param DestTex            Destination texture to copy to
	 */
	void AddCopyPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGTextureRef SourceTex,
		FRDGTextureRef DestTex
	);

	/**
	 * Filtering pass after upsampling.
	 *
	 * @param GraphBuilder			RDG builder
	 * @param RenderData			RenderData ref
	 * @param View					Current scene view
	 * @param SceneTex				SceneColor texture
	 * @param SmokeAlbedo			SmokeAlbedo texture from ray marching
	 * @param SmokeLocalPosAlpha	Smoke (local position, alpha) texture from ray marching
	 * @param TexSize				Output texture size
	 * @param ViewRectMin			ViewRect offset for UV calculation
	 */
	FRDGTextureRef AddUpsampleFilterPass(
		FRDGBuilder& GraphBuilder,
		const FIVSmokePackedRenderData& RenderData,
		const FSceneView& View,
		FRDGTextureRef SceneTex,
		FRDGTextureRef SmokeAlbedo,
		FRDGTextureRef SmokeLocalPosAlpha,
		const FIntPoint& TexSize,
		const FIntPoint& ViewRectMin
	);

	/**
	 * Visual pass after Upsample Filtering.
	 *
	 * @param GraphBuilder				RDG builder
	 * @param RenderData				RenderData ref
	 * @param View						Current scene view
	 * @param SmokeTex					Smoke texture after Upsample filter pass
	 * @param SmokeLocalPosAlphaTex		Smoke (local position, alpha) texture from ray marching
	 * @param SmokeWorldPosDepthTex		Smoke (world position, linear depth) texture from ray marching
	 * @param SceneTex,					Scene texture
	 * @param TexSize					Output texture size
	 */
	FRDGTextureRef AddSmokeVisualPass(FRDGBuilder& GraphBuilder, const FIVSmokePackedRenderData& RenderData, const FSceneView& View, FRDGTextureRef SmokeTex, FRDGTextureRef SmokeLocalPosAlphaTex, FRDGTextureRef SmokeWorldPosDepthTex, FRDGTextureRef SceneTex, const FIntPoint& TexSize);

	/**
	 * Composite PS Pass.
	 * Blends ray marching result (Dual RT) with scene color and applies sharpening/blurring.
	 *
	 * @param GraphBuilder				RDG builder
	 * @param RenderData				RenderData ref
	 * @param View						Current scene view
	 * @param SceneTex					Scene color texture
	 * @param SmokeTex					Smoke texture after Upsample filter pass
	 * @param Output					Final render target
	 * @param ViewportSize				Size of the viewport for UV calculation
	 */
	void AddCompositePass(
		FRDGBuilder& GraphBuilder,
		const FIVSmokePackedRenderData& RenderData,
		const FSceneView& View,
		FRDGTextureRef SceneTex,
		FRDGTextureRef SmokeVisualTex,
		const FScreenPassRenderTarget& Output,
		const FIntPoint& ViewportSize);

	//~==============================================================================
	// Pre-Pass Pipeline (Internal Helpers)

	/**
	 * Check if the view is a primary game view (not a capture or reflection).
	 * Used to filter out Scene Capture, Planar Reflection, etc. for performance.
	 *
	 * @param View  Scene view to check
	 * @return true if this is a primary game view that should render smoke
	 */
	static bool IsPrimaryGameView(const FSceneView& View);

	/**
	 * Execute depth write pass using current frame's ray march results.
	 * Called after ray march in Pre-pass pipeline.
	 *
	 * @param GraphBuilder           RDG builder
	 * @param View                   Current scene view
	 * @param RenderTargets          Render target slots (for depth write)
	 * @param SmokeWorldPosDepthTex  World position + depth texture
	 * @param SmokeLocalPosAlphaTex  Local position + alpha texture
	 * @param ViewportSize           ViewRect.Size() for UV calculation
	 * @param ViewRectMin            ViewRect.Min for UV calculation
	 */
	void ExecuteDepthWrite(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const struct FRenderTargetBindingSlots& RenderTargets,
		FRDGTextureRef SmokeWorldPosDepthTex,
		FRDGTextureRef SmokeLocalPosAlphaTex,
		const FIntPoint& ViewportSize,
		const FIntPoint& ViewRectMin
	);

	//~==============================================================================
	// Per-View Data (Camera-dependent, valid within single GraphBuilder)

	/**
	 * Per-View rendering data for Pre-pass → Post-process data transfer.
	 * Contains ray marching results that are camera-dependent.
	 * Valid only within the same RDG builder (same ViewFamily's frame).
	 *
	 * Cleanup: Cleared when the owning ViewFamily finishes rendering.
	 */
	struct FPerViewData
	{
		FRDGTextureRef SmokeTex = nullptr;           // After UpsampleFilter
		FRDGTextureRef LocalPosAlphaTex = nullptr;   // Full resolution
		FRDGTextureRef WorldPosDepthTex = nullptr;   // Full resolution
		FIntPoint ViewportSize = FIntPoint::ZeroValue;
		FIntPoint ViewRectMin = FIntPoint::ZeroValue;
		bool bIsValid = false;
	};

	/** View → Per-View data map. Each View has independent camera-dependent render results. */
	TMap<const FSceneViewStateInterface*, FPerViewData> ViewDataMap;

	/** Mutex for thread-safe access to ViewDataMap (protects against viewport switching races). */
	mutable FCriticalSection ViewDataMutex;

public:
	/** Clear per-view data for a specific ViewFamily. Called at end of ViewFamily rendering. */
	void ClearViewDataForViewFamily(const FSceneViewFamily& ViewFamily)
	{
		FScopeLock Lock(&ViewDataMutex);

		for (const FSceneView* View : ViewFamily.Views)
		{
			if (View && View->State)
			{
				ViewDataMap.Remove(View->State);
			}
		}
	}

private:

	//~==============================================================================
	// State (Global - Shared across all Worlds)

	/** Shared noise volume texture for all smoke rendering. Prevent GC via AddToRoot. */
	UTextureRenderTargetVolume* NoiseVolume = nullptr;

	/** Re-entry guard to prevent infinite recursion during shadow capture. */
	bool bIsCapturingShadow = false;

	//~==============================================================================
	// Per-World Data Management

	/**
	 * Map of World -> Per-World Data.
	 * Each World (Editor, PIE, Standalone) has its own rendering state.
	 * TObjectKey<UWorld> is used for safe UObject pointer comparison.
	 * TSharedPtr ensures data survives across thread boundaries.
	 *
	 * THREAD SAFETY: All access must be protected by WorldDataMutex.
	 */
	TMap<TObjectKey<UWorld>, TSharedPtr<FPerWorldData>> WorldDataMap;

	/** Mutex for thread-safe access to WorldDataMap. */
	mutable FCriticalSection WorldDataMutex;

	/** Delegate handles for World destruction callbacks. */
	TMap<TObjectKey<UWorld>, FDelegateHandle> WorldEndPlayHandles;

	//~==============================================================================
	// CSM Helpers (Per-World)

	/** Initialize CSM renderer for a specific World if needed. */
	void InitializeCSM(TSharedPtr<FPerWorldData> WorldData, UWorld* World);

	/**
	 * Find the main directional light (Atmosphere Sun Light) in the world.
	 * Uses the same logic as the engine: bAtmosphereSunLight + AtmosphereSunLightIndex.
	 *
	 * @param World          World to search in
	 * @param OutDirection   Direction TOWARD the light source (opposite of light travel direction)
	 * @param OutColor       Light color
	 * @param OutIntensity   Light intensity
	 * @return true if a directional light was found
	 */
	bool GetMainDirectionalLight(UWorld* World, FVector& OutDirection, FLinearColor& OutColor, float& OutIntensity);

	//~==============================================================================
	// Stats Tracking

	/** Last time stats were updated (for 1-second interval). */
	double LastStatUpdateTime = 0.0;

	/** Cached memory sizes for stat reporting. */
	int64 CachedNoiseVolumeSize = 0;
	int64 CachedCSMSize = 0;
	int64 CachedPerFrameSize = 0;

	/** Update stats if 1 second has passed since last update. */
	void UpdateStatsIfNeeded(const FIVSmokePackedRenderData& RenderData, const FIntPoint& ViewportSize);

	/** Calculate per-frame texture memory usage. */
	int64 CalculatePerFrameTextureSize(
		const FIntPoint& ViewportSize,
		int32 VolumeCount,
		const FIntVector& VoxelResolution,
		const FIntVector& HoleResolution
	) const;

	/** Update all stat values. */
	void UpdateAllStats();
};
