// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "IVSmokeGridLibrary.h"
#include "RHI.h"
#include "RHIResources.h"
#include "TimerManager.h"
#include "UObject/ObjectMacros.h"
#include "IVSmokeVoxelVolume.generated.h"

class UBoxComponent;
class UBillboardComponent;
class UIVSmokeCollisionComponent;
class UIVSmokeSmokePreset;
class UIVSmokeHoleGeneratorComponent;

/**
 * Represents the current phase of the smoke simulation lifecycle.
 */
UENUM(BlueprintType)
enum class EIVSmokeVoxelVolumeState : uint8
{
	/** Simulation is inactive. */
	Idle,

	/** Smoke is spreading via flood-fill. */
	Expansion,

	/** Smoke maintains its shape. */
	Sustain,

	/** Smoke is fading out and voxels are being removed. */
	Dissipation,

	/** Simulation has ended. */
	Finished
};

/**
 * Replicated state structure to synchronize simulation timing and random seeds across the network.
 */
USTRUCT(BlueprintType)
struct FIVSmokeServerState
{
	GENERATED_BODY()

	/** Current phase of the simulation state machine. */
	UPROPERTY()
	EIVSmokeVoxelVolumeState State = EIVSmokeVoxelVolumeState::Idle;

	/** World time (synced) when the expansion phase began. */
	UPROPERTY()
	float ExpansionStartTime = 0.0f;

	/** World time (synced) when the sustain phase began. */
	UPROPERTY()
	float SustainStartTime = 0.0f;

	/** World time (synced) when the dissipation phase began. */
	UPROPERTY()
	float DissipationStartTime = 0.0f;

	/** Seed for deterministic procedural generation across clients. */
	UPROPERTY()
	int32 RandomSeed = 0;

	/**
	 * Increments every time the simulation resets.
	 * Used to force clients (including late-joiners) to reset their local state and resync with the server.
	 */
	UPROPERTY()
	uint8 Generation = 0;
};

/**
 * Dirty level for GPU texture synchronization.
 */
UENUM(BlueprintType)
enum class EIVSmokeDirtyLevel : uint8
{
	/** Texture is up-to-date. */
	Clean,

	/** Voxel data changed, texture upload required. */
	Dirty
};

/**
 * Visualization modes for debugging.
 */
UENUM(BlueprintType)
enum class EIVSmokeDebugViewMode : uint8
{
	/** Single uniform color. */
	SolidColor,

	/** Gradient based on generation order. */
	Heatmap
};

/**
 * Editor and runtime debug settings.
 */
USTRUCT(BlueprintType)
struct FIVSmokeDebugSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug")
	bool bDebugEnabled = true;

	/** If true, smoke is rendered during editor preview. Disable to see only debug visualization without smoke. */
	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bRenderSmokeInPreview = true;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled"))
	EIVSmokeDebugViewMode ViewMode = EIVSmokeDebugViewMode::SolidColor;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bShowVolumeBounds = true;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bShowVoxelMesh = false;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bShowVoxelWireframe = true;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bShowStatusText = true;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled", UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0))
	FColor DebugWireframeColor = FColor(20, 20, 20);

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled", UIMin=0.0, UIMax=1.0))
	float SliceHeight = 1.0f;

	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug", meta = (EditCondition = "bDebugEnabled", ClampMin=0, ClampMax=100))
	int32 VisibleStepCountPercent = 100;
};

/**
 * The core volumetric actor that simulates dynamic smoke expansion using a deterministic voxel-based flood-fill algorithm.
 *
 * ## Overview
 * This actor generates a 3D grid of voxels that expand outward from the center, navigating around obstacles
 * defined by the collision settings. The simulation is deterministic, ensuring the same shape and timing
 * across both Server and Clients without replicating individual voxel data.
 *
 * ## Simulation Lifecycle
 * The simulation state machine progresses based on the sum of duration and fade settings:
 * 1. Idle: Initial state.
 * 2. Expansion: Spawns voxels. Ends after `ExpansionDuration + FadeInDuration`.
 * 3. Sustain: Maintains the shape. Ends after `SustainDuration`.
 * 4. Dissipation: Removes voxels. Ends after `DissipationDuration + FadeOutDuration`.
 * 5. Finished: Simulation complete.
 *
 * ## Network & Execution
 * The simulation logic executes deterministically on both the Server and Client.
 * - The Server manages the authoritative state (State, Seed, StartTime) and replicates it to Clients.
 * - Clients execute the exact same flood-fill algorithm locally based on the replicated Seed and Time.
 */
UCLASS()
class IVSMOKE_API AIVSmokeVoxelVolume : public AActor
{
	GENERATED_BODY()

	//~==============================================================================
	// Actor Lifecycle
#pragma region Lifecycle
public:
	AIVSmokeVoxelVolume();

	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	virtual void EditorApplyTranslation(const FVector& DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	virtual bool IsSelectable() const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#pragma endregion

	//~==============================================================================
	// Actor Components
#pragma region Components
public:
	/**
	 * Returns the component responsible for generating holes (negative space) in the smoke.
	 * Caches the result to avoid repeated lookups. Returns nullptr if the component is missing.
	 */
	TObjectPtr<UIVSmokeHoleGeneratorComponent> GetHoleGeneratorComponent();

	/**
	 * Returns the component responsible for handling physical interactions and collision queries.
	 * Caches the result to avoid repeated lookups. Returns nullptr if the component is missing.
	 */
	TObjectPtr<UIVSmokeCollisionComponent> GetCollisionComponent();

	/** Editor visualization sprite. Used as the RootComponent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IVSmoke")
	TObjectPtr<UBillboardComponent> BillboardComponent;

private:
	/**
	 * Component that handles dynamic hole generation.
	 * Reacts to physical interactions (e.g., projectiles, explosions) to carve out holes in the smoke,
	 */
	UPROPERTY(EditAnywhere, Category = "IVSmoke")
	TObjectPtr<UIVSmokeHoleGeneratorComponent> HoleGeneratorComponent;

	/**
	 * Component that manages dynamic collision volumes.
	 * Generates blocking geometry based on active voxels, mainly designed to obstruct AI vision
	 * and prevent them from seeing through the thick smoke.
	 */
	UPROPERTY(EditAnywhere, Category = "IVSmoke")
	TObjectPtr<UIVSmokeCollisionComponent> CollisionComponent;

#if WITH_EDITORONLY_DATA
	/**
	 * InstancedStaticMeshComponent used exclusively for editor-time debug visualization.
	 * Renders individual voxels as meshes when `bShowVoxelMesh` is enabled.
	 */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> DebugMeshComponent;
#endif
#pragma endregion

	//~==============================================================================
	// Actor Configuration
#pragma region Configuation
public:
	/**
	 * Half-size of the voxel grid in index units.
	 * The actual grid resolution will be `(Extent * 2) - 1` per axis.
	 * @note Increasing this value exponentially increases memory usage. Keep it as low as possible.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config", meta = (ClampMin = "1", ClampMax = "16"))
	FIntVector VolumeExtent = FIntVector(16, 16, 16);

	/**
	 * Defines the relative aspect ratio of the smoke's expansion shape per axis.
	 * These values act as ratios, not absolute units.
	 * - Example: (1.0, 1.0, 1.0) creates a spherical shape.
	 * - Example: (2.0, 1.0, 1.0) creates an ellipsoid that stretches twice as far along the X-axis.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "5.0"))
	FVector Radii = FVector(1.0f, 1.0f, 1.0f);

	/**
	 * World space size of a single voxel in centimeters.
	 * @note Larger values cover more area with the same performance cost but reduce visual detail.
	 * Smaller values require more voxels (higher `VolumeExtent` or `MaxVoxelNum`) to cover the same area.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config", meta = (ClampMin = "1.0", UIMin = "10.0", UIMax = "100.0"))
	float VoxelSize = 50.0f;

	/**
	 * The hard limit on the number of active voxels.
	 * Simulation will stop spawning new voxels once this limit is reached.
	 * Use this to guarantee a fixed performance budget for this actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config", meta = (ClampMin = "1", UIMin = "100", UIMax = "10000"))
	int32 MaxVoxelNum = 1000;

	/** If true, the simulation starts automatically on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config")
	bool bAutoStart = false;

	/** If true, the actor is automatically destroyed when the simulation reaches the `Finished` state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config")
	bool bDestroyOnFinish = false;

	/** If true, the smoke stays in the `Sustain` phase indefinitely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config")
	bool bIsInfinite = false;

	/**
	 * Optional Data Asset to override visual properties such as Smoke Color, Absorption, and Density.
	 * If set, the visual settings in this preset take precedence over the actor's local settings.
	 * @see UIVSmokeSmokePreset
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Config")
	TObjectPtr<UIVSmokeSmokePreset> SmokePresetOverride;

#pragma endregion

	//~==============================================================================
	// Flood Fill Simulation
#pragma region Simulation
public:
	/**
	 * Allocates memory for the voxel grid based on `VolumeExtent`.
	 * Automatically called on BeginPlay. Can be called manually to resize the grid at runtime,
	 */
	UFUNCTION(BlueprintCallable, Category = "IVSmoke")
	void Initialize();

	/**
	 * Begins the simulation. (Server Only)
	 * Sets the state to `Expansion` and synchronizes the start time and random seed to clients.
	 * Has no effect if the simulation is already running.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "IVSmoke")
	void StartSimulation();

	/**
	 * Stops the simulation and triggers the dissipation phase. (Server Only)
	 *
	 * @param bImmediate	If true, skips the dissipation phase and instantly transitions to `Finished`, clearing all voxels.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "IVSmoke")
	void StopSimulation(bool bImmediate = false);

	/**
	 * Resets the simulation state to `Idle` and clears all voxel data. (Server Only)
	 * Increments the `Generation` counter to force all clients to reset and resync.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "IVSmoke")
	void ResetSimulation();

	/**
	 * The duration (in seconds) of the active expansion phase where voxels are spawned.
	 * The actual Expansion state lasts for `ExpansionDuration + FadeInDuration`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0"))
	float ExpansionDuration = 3.0f;

	/**
	 * The duration (in seconds) the smoke maintains its shape after expansion.
	 * Ignored if `bIsInfinite` is true.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0"))
	float SustainDuration = 5.0f;

	/**
	 * The duration (in seconds) of the voxel removal phase.
	 * The actual Dissipation state lasts for `DissipationDuration + FadeOutDuration`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0"))
	float DissipationDuration = 2.0f;

	/**
	 * Additional time added to the Expansion phase to allow for opacity fade-in.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0"))
	float FadeInDuration = 2.0f;

	/**
	 * Additional time added to the Dissipation phase to allow for opacity fade-out.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 2.0f;

	/**
	 * Randomness added to the flood-fill pathfinding cost.
	 * Higher values create more irregular, jagged shapes instead of a perfect sphere.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5000.0"))
	float ExpansionNoise = 100.0f;

	/**
	 * Randomness added to the voxel removal order.
	 * Higher values cause the smoke to break apart more randomly during dissipation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5000.0"))
	float DissipationNoise = 100.0f;

	/**
	 * Defines the normalized rate of voxel spawning over `ExpansionDuration`.
	 * - X-axis (Time): 0.0 to 1.0 (Normalized Duration)
	 * - Y-axis (Value): 0.0 to 1.0 (Fraction of `MaxVoxelNum` to spawn)
	 * @note The curve should be monotonically increasing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation")
	TObjectPtr<UCurveFloat> ExpansionCurve;

	/**
	 * Defines the normalized rate of voxel survival over `DissipationDuration`.
	 * - X-axis (Time): 0.0 to 1.0 (Normalized Duration)
	 * - Y-axis (Value): 1.0 to 0.0 (Fraction of voxels remaining)
	 * @note The curve should be monotonically decreasing (start at 1.0, end at 0.0).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation")
	TObjectPtr<UCurveFloat> DissipationCurve;

	/**
	 * If true, voxels perform collision checks against the world before spawning.
	 * Disable this to allow smoke to pass through walls, significantly reducing CPU cost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation")
	bool bEnableSimulationCollision = true;

	/** The collision channel used for obstacle detection during expansion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IVSmoke | Simulation", meta = (EditCondition = "bEnableSimulationCollision"))
	TEnumAsByte<ECollisionChannel> VoxelCollisionChannel = ECC_WorldStatic;

private:
	/** Internal node structure for the Dijkstra-based flood fill algorithm. */
	struct FIVSmokeVoxelNode
	{
		int32 Index;
		int32 ParentIndex;
		float Cost;
		bool operator<(const FIVSmokeVoxelNode& Other) const
		{
			if (FMath::IsNearlyEqual(Cost, Other.Cost))
			{
				return Index < Other.Index;
			}
			return Cost < Other.Cost;
		}
	};

	/**
	 * Helper to sample a curve or return linear alpha if no curve is provided.
	 *
	 * @param ElapsedTime	Current time elapsed in the phase.
	 * @param Duration		Total duration of the phase.
	 * @param Curve			Optional curve to sample. If null, returns linear interpolation (ElapsedTime / Duration).
	 * @return				Clamped float value between 0.0 and 1.0.
	 */
	FORCEINLINE static float GetCurveValue(float ElapsedTime, float Duration, const UCurveFloat* Curve)
	{
		if (Duration <= KINDA_SMALL_NUMBER)
		{
			return 1.0f;
		}

		float Alpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

		if (Curve)
		{
			return FMath::Clamp(Curve->GetFloatValue(Alpha), 0.0f, 1.0f);
		}

		return Alpha;
	}

	/** Handles network replication of the simulation state. */
	UFUNCTION()
	void OnRep_ServerState();

	/**
	 * Main state machine handler.
	 * Transitions the local simulation logic to the new state (e.g., resets heaps, clears data).
	 *
	 * @param NewState		The state to transition to.
	 */
	void HandleStateTransition(EIVSmokeVoxelVolumeState NewState);

	/** Resets all internal simulation arrays and counters to their initial state. */
	void ClearSimulationData();

	/**
	 * Checks if the line of sight between two voxel centers is blocked.
	 *
	 * @param World			Pointer to the world context.
	 * @param BeginPos		Start position of the trace.
	 * @param EndPos		End position of the trace.
	 * @return				True if a blocking hit occurs between the positions.
	 */
	bool IsConnectionBlocked(const UWorld* World, const FVector& BeginPos, const FVector& EndPos) const;

	/**
	 * Core logic for starting the simulation.
	 * Separated from the RPC to allow execution in both Editor-Preview and Networked-Server contexts.
	 */
	void StartSimulationInternal();

	/**
	 * Core logic for stopping or dissipating the smoke.
	 *
	 * @param bImmediate	If true, skips the dissipation phase and instantly transitions to `Finished`, clearing all voxels.
	 */
	void StopSimulationInternal(bool bImmediate = false);

	/**
	 * Core logic for resetting the entire simulation state.
	 * Clears buffers, resets generation, and returns the actor to an Idle state.
	 */
	void ResetSimulationInternal();

	/**
	 * Simulates frames rapidly to catch up with the server's current state.
	 * Called on clients when they detect a `Generation` mismatch (late join or reset).
	 */
	void FastForwardSimulation();

	/** Per-frame update logic for the Expansion phase. */
	void UpdateExpansion();

	/** Per-frame update logic for the Sustain phase. */
	void UpdateSustain();

	/** Per-frame update logic for the Dissipation phase. */
	void UpdateDissipation();

	/**
	 * Pops nodes from the ExpansionHeap and spawns new voxels.
	 *
	 * @param SpawnNum		Number of voxels to spawn this frame.
	 * @param StartSimTime	Simulation time at the beginning of the frame.
	 * @param EndSimTime	Simulation time at the end of the frame.
	 */
	void ProcessExpansion(int32 SpawnNum, float StartSimTime, float EndSimTime);

	/**
	 * Pops nodes from the DissipationHeap and removes existing voxels.
	 *
	 * @param RemoveNum		Number of voxels to remove this frame.
	 * @param StartSimTime	Simulation time at the beginning of the frame.
	 * @param EndSimTime	Simulation time at the end of the frame.
	 */
	void ProcessDissipation(int32 RemoveNum, float StartSimTime, float EndSimTime);

	/**
	 * Sets the birth time for a voxel and marks it as active.
	 *
	 * @param Index			Index of the voxel in the grid array.
	 * @param BirthTime		The simulation time when this voxel was created.
	 */
	void SetVoxelBirthTime(int32 Index, float BirthTime);

	/**
	 * Sets the death time for a voxel and marks it as inactive.
	 *
	 * @param Index			Index of the voxel in the grid array.
	 * @param DeathTime		The simulation time when this voxel was removed.
	 */
	void SetVoxelDeathTime(int32 Index, float DeathTime);

	/**
	 * Calculates the weighted Euclidean distance between two voxels based on Radii.
	 * Used for Theta* cost estimation to approximate the actual distance.
	 *
	 * @param IndexA	Index of the first voxel.
	 * @param IndexB	Index of the second voxel.
	 * @param InvRadii	Inverse of the radii vector for weighting the distance per axis.
	 * @return			The weighted distance between the two voxels.
	 */
	float CalculateWeightedDistance(int32 IndexA, int32 IndexB, const FVector& InvRadii) const;

	/** Replicated state synchronized from the server. */
	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FIVSmokeServerState ServerState;

	/** Local copy of the state machine to detect transitions. */
	EIVSmokeVoxelVolumeState LocalState = EIVSmokeVoxelVolumeState::Idle;

	/** Tracks the generation number locally to detect server resets. */
	uint8 LocalGeneration = 0;

	/** RNG stream for deterministic procedural generation. */
	FRandomStream RandomStream;

	/** Current local simulation time relative to the phase start time. */
	float SimTime = 0.0f;

	/** True if memory has been allocated via Initialize(). */
	bool bIsInitialized = false;

	/** True if currently running the fast-forward catch-up logic. */
	bool bIsFastForwarding = false;

	/** World-space bounding box minimum of all active voxels. */
	FVector VoxelWorldAABBMin = FVector(FLT_MAX, FLT_MAX, FLT_MAX);

	/** World-space bounding box maximum of all active voxels. */
	FVector VoxelWorldAABBMax = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	/** Timestamp when each voxel was spawned. */
	TArray<float> VoxelBirthTimes;

	/** Timestamp when each voxel was removed. */
	TArray<float> VoxelDeathTimes;

	/** Pathfinding cost for each voxel index (Dijkstra). */
	TArray<float> VoxelCosts;

	/**
	 * Bitmask buffer representing active voxels, packed for memory efficiency.
	 *
	 * ## Data Layout
	 * Each `uint64` element represents a single row of voxels along the X-axis at a specific (Y, Z) coordinate.
	 * - The X-coordinate maps directly to the bit index (0-63).
	 * - Array Index = `Z * GridResolution.Y + Y`
	 *
	 * @warning Hard Constraint: Since the X-axis is packed into a 64-bit integer, `GridResolution.X` cannot exceed 64.
	 * Consequently, `VolumeExtent.X` is effectively limited to roughly 32 (since Resolution = Extent * 2 - 1).
	 */
	TArray<uint64> VoxelBits;

	/** Priority queue for expansion (lowest cost first). */
	TArray<FIVSmokeVoxelNode> ExpansionHeap;

	/** Priority queue for dissipation (lowest cost + noise first). */
	TArray<FIVSmokeVoxelNode> DissipationHeap;

	/** List of indices of all currently active voxels. */
	TArray<int32> GeneratedVoxelIndices;

	TBitArray<> VoxelPenetrationFlags;

#pragma endregion

	//~==============================================================================
	// Collision
#pragma region Collision
	/**
	 * Updates the collision geometry to match the current state of the voxel grid.
	 * Delegates the actual mesh/body generation to the `CollisionComponent`.
	 *
	 * @param bForce	If true, forces a geometry rebuild even if the voxel data hasn't changed.
	 *					Used during initialization or when applying a new preset.
	 */
	void TryUpdateCollision(bool bForce = false);
#pragma endregion

	//~==============================================================================
	// Data Access
#pragma region DataAccess
public:
	/** Returns the current phase of the simulation state machine. */
	FORCEINLINE EIVSmokeVoxelVolumeState GetCurrentState() const { return ServerState.State; }

	/**
	 * Returns true if this volume should be rendered.
	 * Used by SceneViewExtension to filter active volumes without explicit registration.
	 */
	bool ShouldRender() const;

	/** Returns the raw array of timestamps indicating when each voxel was created (Server Time). */
	FORCEINLINE const TArray<float>& GetVoxelBirthTimes() const { return VoxelBirthTimes; }

	/** Returns the raw array of timestamps indicating when each voxel was removed (Server Time). */
	FORCEINLINE const TArray<float>& GetVoxelDeathTimes() const { return VoxelDeathTimes; }

	/** Returns the grid resolution (dimensions of the voxel grid). */
	FORCEINLINE FIntVector GetGridResolution() const
	{
		FIntVector GridResolution;
		GridResolution.X = FMath::Max(1, (VolumeExtent.X * 2) - 1);
		GridResolution.Y = FMath::Max(1, (VolumeExtent.Y * 2) - 1);
		GridResolution.Z = FMath::Max(1, (VolumeExtent.Z * 2) - 1);
		return GridResolution;
	}

	/** Returns the center offset for grid-to-local coordinate conversion. */
	FORCEINLINE FIntVector GetCenterOffset() const { return VolumeExtent - FIntVector(1, 1, 1); }

	/** Returns the world-space size of each voxel. */
	FORCEINLINE float GetVoxelSize() const { return VoxelSize; }

	/** Returns the current dirty level for GPU buffer synchronization. */
	FORCEINLINE EIVSmokeDirtyLevel GetDirtyLevel() const { return DirtyLevel; }

	/** Returns true if voxel data has been modified since last GPU upload. */
	FORCEINLINE bool IsVoxelDataDirty() const { return DirtyLevel != EIVSmokeDirtyLevel::Clean; }

	/**
	 * Marks the voxel data as clean after a successful GPU upload.
	 * @note Should only be called by the `IVSmokeRenderer`.
	 */
	FORCEINLINE void ClearVoxelDataDirty() { DirtyLevel = EIVSmokeDirtyLevel::Clean; }

	/** Returns the current buffer size (for detecting resize). */
	FORCEINLINE int32 GetVoxelBufferSize() const { return VoxelBirthTimes.Num(); }

	/** Returns the number of active (non-zero density) voxels. */
	FORCEINLINE int32 GetActiveVoxelNum() const { return ActiveVoxelNum; }

	/** Returns the smoke preset override for this volume, or nullptr to use default. */
	FORCEINLINE const UIVSmokeSmokePreset* GetSmokePresetOverride() const { return SmokePresetOverride; }

	/** Returns the AABBMin of voxels. */
	FORCEINLINE FVector GetVoxelWorldAABBMin() const { return VoxelWorldAABBMin - VoxelSize; }

	/** Returns the AABBMax of voxels. */
	FORCEINLINE FVector GetVoxelWorldAABBMax() const { return VoxelWorldAABBMax + VoxelSize; }

	/**
	 * Checks if a voxel at the given linear index is currently active.
	 *
	 * @param Index		Linear index of the voxel.
	 * @return			True if the voxel is active (bit set).
	 */
	FORCEINLINE bool IsVoxelActive(int32 Index) const
	{
		FIntVector GridPos = UIVSmokeGridLibrary::IndexToGrid(Index, GetGridResolution());
		return IsVoxelActive(GridPos);
	}

	/**
	 * Checks if a voxel at the given grid coordinate is currently active.
	 *
	 * @param GridPos	3D grid coordinate of the voxel.
	 * @return			True if the voxel is active.
	 */
	FORCEINLINE bool IsVoxelActive(FIntVector GridPos) const
	{
		return UIVSmokeGridLibrary::IsVoxelBitSet(VoxelBits, GridPos, GetGridResolution());
	}

	/** Returns the RHI texture resource from the HoleGeneratorComponent, if available. */
	FTextureRHIRef GetHoleTexture() const;

	/**
	 * Returns the synchronized world time in seconds.
	 * Handles network time offsets to ensure clients see the simulation at the same progress as the server.
	 */
	float GetSyncWorldTimeSeconds() const;

private:
	/** Internal counter for the number of active voxels. */
	int32 ActiveVoxelNum = 0;

	/** Tracks changes to voxel data for render thread synchronization. */
	EIVSmokeDirtyLevel DirtyLevel = EIVSmokeDirtyLevel::Clean;
#pragma endregion

	//~==============================================================================
	// Debug
#pragma region Debug
public:
	/**
	 * Runs a preview of the simulation directly in the Editor viewport without starting a Play In Editor (PIE) session.
	 * Use this to quickly iterate on parameters like `ExpansionDuration`, `Noise`, and `Curves`.
	 * @note This resets the current simulation state.
	 */
	UFUNCTION(CallInEditor, Category = "IVSmoke | Debug")
	void StartPreviewSimulation();

	/**
	 * Stops the current editor preview simulation and clears all generated voxel data.
	 * Returns the actor to an Idle state.
	 */
	UFUNCTION(CallInEditor, Category = "IVSmoke | Debug")
	void StopPreviewSimulation();

	/** Configuration settings for visual debugging tools. */
	UPROPERTY(EditAnywhere, Category = "IVSmoke | Debug")
	FIVSmokeDebugSettings DebugSettings;

	/** Optional static mesh to use for voxel visualization when `bShowVoxelMesh` is enabled in settings. */
	UPROPERTY(EditDefaultsOnly, Category = "IVSmoke | Debug", AdvancedDisplay)
	TObjectPtr<UStaticMesh> DebugVoxelMesh;

	/** Optional material to apply to the debug voxel mesh. */
	UPROPERTY(EditDefaultsOnly, Category = "IVSmoke | Debug", AdvancedDisplay)
	TObjectPtr<UMaterialInterface> DebugVoxelMaterial;

private:
	/** Main entry point for drawing all enabled debug visualizations per frame. */
	void DrawDebugVisualization() const;

	/** Draws voxel volume bounds. */
	void DrawDebugVolumeBounds() const;

	/** Draws lightweight wireframe cubes for active voxels. */
	void DrawDebugVoxelWireframes() const;

	/** Draws instanced static meshes for active voxels. Heavier performance cost. */
	void DrawDebugVoxelMeshes() const;

	/** Displays world-space text showing the current State, Voxel Count, and Simulation Time. */
	void DrawDebugStatusText() const;

	/** Calculates a CRC32 checksum of the current voxel state to verify deterministic sync between Server and Client. */
	uint32 CalculateSimulationChecksum() const;

	/** Internal flag to track if the actor is currently running an editor-only preview simulation. */
	bool bIsEditorPreviewing = false;
#pragma endregion
};
