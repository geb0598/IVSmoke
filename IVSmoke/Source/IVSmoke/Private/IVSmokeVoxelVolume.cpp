// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeVoxelVolume.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "IVSmoke.h"
#include "IVSmokeCollisionComponent.h"
#include "IVSmokeGridLibrary.h"
#include "IVSmokeHoleGeneratorComponent.h"
#include "Components/BillboardComponent.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DECLARE_CYCLE_STAT(TEXT("Update Expansion"),	STAT_IVSmoke_UpdateExpansion,		STATGROUP_IVSmoke);
DECLARE_CYCLE_STAT(TEXT("Update Sustain"),		STAT_IVSmoke_UpdateSustain,			STATGROUP_IVSmoke);
DECLARE_CYCLE_STAT(TEXT("Update Dissipation"),	STAT_IVSmoke_UpdateDissipation,		STATGROUP_IVSmoke);
DECLARE_CYCLE_STAT(TEXT("Process Expansion"),	STAT_IVSmoke_ProcessExpansion,		STATGROUP_IVSmoke);
DECLARE_CYCLE_STAT(TEXT("Prepare Dissipation"),	STAT_IVSmoke_PrepareDissipation,	STATGROUP_IVSmoke);
DECLARE_CYCLE_STAT(TEXT("Process Dissipation"),	STAT_IVSmoke_ProcessDissipation,	STATGROUP_IVSmoke);

DECLARE_DWORD_COUNTER_STAT(TEXT("Active Voxel Count"),					STAT_IVSmoke_ActiveVoxelCount,	STATGROUP_IVSmoke);
DECLARE_DWORD_COUNTER_STAT(TEXT("Created Voxel Count (Per Frame)"),		STAT_IVSmoke_CreatedVoxel,		STATGROUP_IVSmoke);
DECLARE_DWORD_COUNTER_STAT(TEXT("Destroyed Voxel Count (Per Frame)"),	STAT_IVSmoke_DestroyedVoxel,	STATGROUP_IVSmoke);

namespace IVSmokeVoxelVolumeCVars
{
	static void ForEachVoxelVolume(UWorld* World, TFunctionRef<void(AIVSmokeVoxelVolume*)> Func)
	{
		if (!World)
		{
			return;
		}

		for (TActorIterator<AIVSmokeVoxelVolume> Iter(World); Iter; ++Iter)
		{
			if (AIVSmokeVoxelVolume* Volume = *Iter)
			{
				Func(Volume);
			}
		}
	}

	static void SetVoxelVolumeDebug(const TArray<FString>& Args, UWorld* World, bool FIVSmokeDebugSettings::* MemberProp, const TCHAR* PropName)
	{
		int32 UpdatedCount = 0;
		bool bLastState = false;

		ForEachVoxelVolume(World, [&](AIVSmokeVoxelVolume* Volume)
		{
			bool& bValue = Volume->DebugSettings.*MemberProp;

			bool bNewValue = (Args.Num() > 0 ? Args[0].ToBool() : !bValue);
			bValue = bNewValue;
			bLastState = bNewValue;

			UpdatedCount++;
		});

#if WITH_EDITOR
		if (UpdatedCount > 0 && Args.Num() == 0)
		{
			UE_LOG(LogIVSmoke, Log, TEXT("[IVSmoke.Volume] %s toggled to: %s (%d Actors updated)"),
				PropName, bLastState ? TEXT("ON") : TEXT("OFF"), UpdatedCount);
		}
#endif
	}

	static FAutoConsoleCommandWithWorldAndArgs Cmd_Volume_Debug(
		TEXT("IVSmoke.Volume.Debug"),
		TEXT("Master toggle for Voxel Volume debug visualization.\nUsage: IVSmoke.Volume.Debug [0/1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetVoxelVolumeDebug(Args, World, &FIVSmokeDebugSettings::bDebugEnabled, TEXT("DebugEnabled"));
		})
	);

	static FAutoConsoleCommandWithWorldAndArgs Cmd_Volume_ShowWireframe(
		TEXT("IVSmoke.Volume.ShowWireframe"),
		TEXT("Toggle wireframe cubes for active voxels.\nUsage: IVSmoke.Volume.ShowWireframe [0/1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetVoxelVolumeDebug(Args, World, &FIVSmokeDebugSettings::bShowVoxelWireframe, TEXT("ShowVoxelWireframe"));
		})
	);

	static FAutoConsoleCommandWithWorldAndArgs Cmd_Volume_ShowMesh(
		TEXT("IVSmoke.Volume.ShowMesh"),
		TEXT("Toggle instanced static meshes for voxels (Expensive).\nUsage: IVSmoke.Volume.ShowMesh [0/1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetVoxelVolumeDebug(Args, World, &FIVSmokeDebugSettings::bShowVoxelMesh, TEXT("ShowVoxelMesh"));
		})
	);

	static FAutoConsoleCommandWithWorldAndArgs Cmd_Volume_ShowStatus(
		TEXT("IVSmoke.Volume.ShowStatus"),
		TEXT("Toggle floating status text above the volume.\nUsage: IVSmoke.Volume.ShowStatus [0/1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetVoxelVolumeDebug(Args, World, &FIVSmokeDebugSettings::bShowStatusText, TEXT("ShowStatusText"));
		})
	);

	static FAutoConsoleCommandWithWorldAndArgs Cmd_Volume_SetViewMode(
		TEXT("IVSmoke.Volume.SetViewMode"),
		TEXT("Set visualization mode.\n0: SolidColor, 1: Heatmap\nUsage: IVSmoke.Volume.SetViewMode <ModeID>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0) return;

			int32 ModeIndex = FCString::Atoi(*Args[0]);
			EIVSmokeDebugViewMode NewMode = (ModeIndex == 1) ? EIVSmokeDebugViewMode::Heatmap : EIVSmokeDebugViewMode::SolidColor;

			ForEachVoxelVolume(World, [NewMode](AIVSmokeVoxelVolume* Volume)
			{
				Volume->DebugSettings.ViewMode = NewMode;
			});

			UE_LOG(LogIVSmoke, Log, TEXT("[IVSmoke.Volume] ViewMode changed to: %s"),
				(NewMode == EIVSmokeDebugViewMode::Heatmap) ? TEXT("Heatmap") : TEXT("SolidColor"));
		})
	);
}

static const FIntVector FloodFillDirections[] = {
	FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
	FIntVector(0, 1, 0), FIntVector(0, -1, 0),
	FIntVector(0, 0, 1), FIntVector(0, 0, -1)
};

//~==============================================================================
// Actor Lifecycle
#pragma region Lifecycle
AIVSmokeVoxelVolume::AIVSmokeVoxelVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Tags.Add(IVSmokeVoxelVolumeTag);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));

	RootComponent = BillboardComponent;

#if WITH_EDITORONLY_DATA
	BillboardComponent->SetEditorScale(1.0f);

	ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/IVSmoke/Icons/T_IVSmoke_VoxelVolumeIcon.T_IVSmoke_VoxelVolumeICon"));
	if (IconTexture.Succeeded())
	{
		BillboardComponent->SetSprite(IconTexture.Object);
	}
#endif

#if WITH_EDITORONLY_DATA
	DebugMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DebugMeshComponent"));
	DebugMeshComponent->SetupAttachment(RootComponent);
	DebugMeshComponent->SetCastShadow(false);
	DebugMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugMeshComponent->SetGenerateOverlapEvents(false);
	DebugMeshComponent->NumCustomDataFloats = 1;
#endif
}

void AIVSmokeVoxelVolume::BeginPlay()
{
#if WITH_EDITOR
	// Lock all Transform changes during PIE to prevent World Partition errors
	// caused by CollisionComponent's dynamic BodySetup
	bLockLocation = true;
#endif

	if (HasAuthority())
	{
		ServerState = FIVSmokeServerState();
	}

	Initialize();

	ClearSimulationData();

	Super::BeginPlay();

	HoleGeneratorComponent = FindComponentByClass<UIVSmokeHoleGeneratorComponent>();

	CollisionComponent = FindComponentByClass<UIVSmokeCollisionComponent>();

	if (HasAuthority())
	{
		if (bAutoStart)
		{
			StartSimulation();
		}
	}
}

void AIVSmokeVoxelVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Reset state so ShouldRender() returns false (prevents rendering after PIE exit)
	ServerState.State = EIVSmokeVoxelVolumeState::Idle;

	Super::EndPlay(EndPlayReason);
}

void AIVSmokeVoxelVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIVSmokeVoxelVolume, ServerState);
}

void AIVSmokeVoxelVolume::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		if (World->GetGameState() == nullptr)
		{
			return;
		}
	}

	Super::Tick(DeltaTime);

	if (ActiveVoxelNum > 0)
	{
		INC_DWORD_STAT_BY(STAT_IVSmoke_ActiveVoxelCount, ActiveVoxelNum);
	}

	switch (ServerState.State)
	{
	case EIVSmokeVoxelVolumeState::Expansion:
		UpdateExpansion();
		break;
	case EIVSmokeVoxelVolumeState::Sustain:
		UpdateSustain();
		break;
	case EIVSmokeVoxelVolumeState::Dissipation:
		UpdateDissipation();
		break;
	case EIVSmokeVoxelVolumeState::Finished:
		[[fallthrough]];
	case EIVSmokeVoxelVolumeState::Idle:
		[[fallthrough]];
	default:
		break;
	}

	TryUpdateCollision();

#if WITH_EDITOR
	if (DebugSettings.bDebugEnabled)
	{
		DrawDebugVisualization();
	}
	else if (DebugMeshComponent && DebugMeshComponent->GetInstanceCount() > 0)
	{
		DebugMeshComponent->ClearInstances();
	}
#endif
}

bool AIVSmokeVoxelVolume::ShouldTickIfViewportsOnly() const
{
	if (GetWorld() != nullptr && GetWorld()->WorldType == EWorldType::Editor && DebugSettings.bDebugEnabled)
	{
		return bIsEditorPreviewing;
	}
	return false;
}

void AIVSmokeVoxelVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

#if WITH_EDITOR
void AIVSmokeVoxelVolume::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	bool bStructuralChange =
			PropertyName == GET_MEMBER_NAME_CHECKED(AIVSmokeVoxelVolume, VolumeExtent)		||
			PropertyName == GET_MEMBER_NAME_CHECKED(AIVSmokeVoxelVolume, MaxVoxelNum);

	bool bParamChange =
			PropertyName == GET_MEMBER_NAME_CHECKED(AIVSmokeVoxelVolume, VoxelSize)			||
			PropertyName == GET_MEMBER_NAME_CHECKED(AIVSmokeVoxelVolume, Radii)				||
			PropertyName == GET_MEMBER_NAME_CHECKED(AIVSmokeVoxelVolume, ExpansionNoise)	||
			PropertyName == GET_MEMBER_NAME_CHECKED(AIVSmokeVoxelVolume, DissipationNoise);

	// Handle bDebugEnabled toggle: stop preview if disabled during preview
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FIVSmokeDebugSettings, bDebugEnabled))
	{
		if (!DebugSettings.bDebugEnabled && bIsEditorPreviewing)
		{
			StopPreviewSimulation();
		}
	}

	if (DebugSettings.bDebugEnabled)
	{
		if (bStructuralChange || bParamChange)
		{
			StopPreviewSimulation();
			StartPreviewSimulation();
		}
	}
}

void AIVSmokeVoxelVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished && DebugSettings.bDebugEnabled)
	{
		StartPreviewSimulation();
	}
}

bool AIVSmokeVoxelVolume::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	if (!InProperty)
	{
		return true;
	}

	// Block Rotation and Scale editing in Details panel
	const FName PropertyName = InProperty->GetFName();
	if (PropertyName == TEXT("RelativeRotation") ||
		PropertyName == TEXT("RelativeScale3D"))
	{
		return false;
	}

	return true;
}

void AIVSmokeVoxelVolume::EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	// Block rotation via gizmo - do nothing
	UE_LOG(LogIVSmoke, Warning, TEXT("[AIVSmokeVoxelVolume] Rotation is not supported."));
}

void AIVSmokeVoxelVolume::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	// Block scale via gizmo - do nothing
	UE_LOG(LogIVSmoke, Warning, TEXT("[AIVSmokeVoxelVolume] Scale is not supported."));
}

void AIVSmokeVoxelVolume::EditorApplyTranslation(const FVector& DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	// Block translation during PIE (editor world actor doesn't have bLockLocation set)
	if (GEditor && GEditor->IsPlaySessionInProgress())
	{
		return;
	}

	Super::EditorApplyTranslation(DeltaTranslation, bAltDown, bShiftDown, bCtrlDown);
}

bool AIVSmokeVoxelVolume::IsSelectable() const
{
	return Super::IsSelectable();
}
#endif

#pragma endregion

//~==============================================================================
// Flood Fill Simulation
#pragma region Simulation

void AIVSmokeVoxelVolume::Initialize()
{
	FIntVector GridResolution = GetGridResolution();

	const int32 TotalGridSizeYZ = GridResolution.Y * GridResolution.Z;
	const int32 TotalGridSize = GridResolution.X * TotalGridSizeYZ;

	if (VoxelBirthTimes.Num() != TotalGridSize)
	{
		VoxelBirthTimes.SetNumZeroed(TotalGridSize);
	}

	if (VoxelDeathTimes.Num() != TotalGridSize)
	{
		VoxelDeathTimes.SetNumZeroed(TotalGridSize);
	}

	if (VoxelCosts.Num() != TotalGridSize)
	{
		VoxelCosts.SetNumUninitialized(TotalGridSize);
	}

	if (VoxelBits.Num() != TotalGridSizeYZ)
	{
		VoxelBits.SetNumUninitialized(TotalGridSizeYZ);
	}

	if (VoxelPenetrationFlags.Num() != TotalGridSize)
	{
		VoxelPenetrationFlags.Init(false, TotalGridSize);
	}

	GeneratedVoxelIndices.Reserve(MaxVoxelNum);

	ExpansionHeap.Reserve(MaxVoxelNum);
	DissipationHeap.Reserve(MaxVoxelNum);

	bIsInitialized = true;
}

void AIVSmokeVoxelVolume::StartSimulation_Implementation()
{
	StartSimulationInternal();
}

void AIVSmokeVoxelVolume::StopSimulation_Implementation(bool bImmediate)
{
	StopSimulationInternal(bImmediate);
}

void AIVSmokeVoxelVolume::ResetSimulation_Implementation()
{
	ResetSimulationInternal();
}

void AIVSmokeVoxelVolume::OnRep_ServerState()
{
	UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		AGameStateBase* GameState = World->GetGameState();

		if (!bIsInitialized || !GameState || GameState->GetServerWorldTimeSeconds() == 0.0f)
		{
			FTimerHandle RetryHandle;
			World->GetTimerManager().SetTimer(RetryHandle, this, &AIVSmokeVoxelVolume::OnRep_ServerState, 0.1f, false);

			UE_LOG(LogIVSmoke, Warning, TEXT("[AIVSmokeVoxelVolume::OnRep_ServerState] GameState not ready yet. Retrying in 0.1s..."));
			return;
		}
	}

	if (LocalGeneration != ServerState.Generation)
	{
		FastForwardSimulation();

		LocalGeneration = ServerState.Generation;

		TryUpdateCollision(true);

		return;
	}

	HandleStateTransition(ServerState.State);
}

void AIVSmokeVoxelVolume::HandleStateTransition(EIVSmokeVoxelVolumeState NewState)
{
	if (LocalState == NewState)
	{
		return;
	}

	SimTime = 0.0f;

	switch (NewState)
	{
	case EIVSmokeVoxelVolumeState::Idle:
		ClearSimulationData();
		break;
	case EIVSmokeVoxelVolumeState::Expansion:
	{
		if (LocalState != EIVSmokeVoxelVolumeState::Idle &&
			LocalState != EIVSmokeVoxelVolumeState::Finished)
		{
			ClearSimulationData();
		}

		RandomStream.Initialize(ServerState.RandomSeed);

		int32 CenterIndex = UIVSmokeGridLibrary::GridToIndex(GetCenterOffset(), GetGridResolution());

		if (VoxelCosts.IsValidIndex(CenterIndex))
		{
			VoxelCosts[CenterIndex] = 0.0f;
			ExpansionHeap.HeapPush({CenterIndex, INDEX_NONE, 0.0f});

		}
		break;
	}
	case EIVSmokeVoxelVolumeState::Sustain:
		TryUpdateCollision(true);
		break;
	case EIVSmokeVoxelVolumeState::Dissipation:
		break;
	case EIVSmokeVoxelVolumeState::Finished:
		if (bDestroyOnFinish)
		{
			if (GetWorld() && GetWorld()->IsGameWorld())
			{
				Destroy();
			}
			else
			{
				ClearSimulationData();
				bIsEditorPreviewing = false;
			}
		}
		ClearSimulationData();
		break;
	}

	LocalState = NewState;
}

void AIVSmokeVoxelVolume::ClearSimulationData()
{
	if (!bIsInitialized)
	{
		Initialize();
	}

	FIntVector GridResolution = GetGridResolution();

	const int32 TotalGridSizeYZ = GridResolution.Y * GridResolution.Z;
	const int32 TotalGridSize = GridResolution.X * TotalGridSizeYZ;

	if (VoxelBirthTimes.Num() != TotalGridSize || VoxelDeathTimes.Num() != TotalGridSize || VoxelBits.Num() != TotalGridSizeYZ)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[ClearSimulationData] Buffer size mismatch detected. Re-initializing..."));
		Initialize();
	}

	FMemory::Memzero(VoxelBirthTimes.GetData(), VoxelBirthTimes.Num() * sizeof(float));

	FMemory::Memzero(VoxelDeathTimes.GetData(), VoxelDeathTimes.Num() * sizeof(float));

	FMemory::Memzero(VoxelBits.GetData(), VoxelBits.Num() * sizeof(uint64));

	VoxelPenetrationFlags.Reset();
	VoxelPenetrationFlags.Init(false, VoxelCosts.Num());

	VoxelCosts.Init(FLT_MAX, VoxelCosts.Num());

	GeneratedVoxelIndices.Reset();

	ExpansionHeap.Reset();
	DissipationHeap.Reset();

	ActiveVoxelNum = 0;
	SimTime = 0.0f;
	DirtyLevel = EIVSmokeDirtyLevel::Dirty;

	if (CollisionComponent)
	{
		CollisionComponent->ResetCollision();
	}

	if (HoleGeneratorComponent)
	{
		HoleGeneratorComponent->Reset();
	}
}

bool AIVSmokeVoxelVolume::IsConnectionBlocked(const UWorld* World, const FVector& BeginPos, const FVector& EndPos) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::AIVSmokeVoxelVolume::IsConnectionBlocked");

	if (!bEnableSimulationCollision)
	{
		return false;
	}

	if (!World)
	{
		return false;
	}

	FCollisionQueryParams CollisionParams;
	CollisionParams.bTraceComplex = false;
	CollisionParams.AddIgnoredActor(this);

	FHitResult HitResult;
	return World->LineTraceSingleByChannel(
		HitResult,
		BeginPos,
		EndPos,
		VoxelCollisionChannel,
		CollisionParams
	);
}

void AIVSmokeVoxelVolume::StartSimulationInternal()
{
	if (!bIsInitialized)
	{
		Initialize();
	}

	ResetSimulationInternal();

	ServerState.RandomSeed = FMath::Rand();
	ServerState.ExpansionStartTime = GetSyncWorldTimeSeconds();

	ServerState.SustainStartTime = 0.0f;
	ServerState.DissipationStartTime = 0.0f;

	ServerState.State = EIVSmokeVoxelVolumeState::Expansion;

	HandleStateTransition(ServerState.State);
}

void AIVSmokeVoxelVolume::StopSimulationInternal(bool bImmediate)
{
	if (ServerState.State == EIVSmokeVoxelVolumeState::Finished)
	{
		return;
	}

	if (bImmediate)
	{
		ServerState.State = EIVSmokeVoxelVolumeState::Finished;
	}
	else if (ServerState.State == EIVSmokeVoxelVolumeState::Expansion ||
			 ServerState.State == EIVSmokeVoxelVolumeState::Sustain)
	{
		ServerState.State = EIVSmokeVoxelVolumeState::Dissipation;
		ServerState.DissipationStartTime = GetSyncWorldTimeSeconds();
	}

	HandleStateTransition(ServerState.State);
}

void AIVSmokeVoxelVolume::ResetSimulationInternal()
{
	ServerState.State = EIVSmokeVoxelVolumeState::Idle;
	ServerState.Generation += 1;

	ServerState.ExpansionStartTime = 0.0f;
	ServerState.SustainStartTime = 0.0f;
	ServerState.DissipationStartTime = 0.0f;

	ClearSimulationData();
	LocalState = EIVSmokeVoxelVolumeState::Idle;
}

void AIVSmokeVoxelVolume::FastForwardSimulation()
{
	bIsFastForwarding = true;

	if (ServerState.State == EIVSmokeVoxelVolumeState::Expansion	||
		ServerState.State == EIVSmokeVoxelVolumeState::Sustain		||
		ServerState.State == EIVSmokeVoxelVolumeState::Dissipation)
	{
		HandleStateTransition(EIVSmokeVoxelVolumeState::Expansion);
		UpdateExpansion();
	}
	if (ServerState.State == EIVSmokeVoxelVolumeState::Sustain ||
		ServerState.State == EIVSmokeVoxelVolumeState::Dissipation)
	{
		HandleStateTransition(EIVSmokeVoxelVolumeState::Sustain);
		UpdateSustain();
	}
	if (ServerState.State == EIVSmokeVoxelVolumeState::Dissipation)
	{
		HandleStateTransition(EIVSmokeVoxelVolumeState::Dissipation);
		UpdateDissipation();
	}

	HandleStateTransition(ServerState.State);

	bIsFastForwarding = false;
}

void AIVSmokeVoxelVolume::UpdateExpansion()
{
	SCOPE_CYCLE_COUNTER(STAT_IVSmoke_UpdateExpansion);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::AIVSmokeVoxelVolume::UpdateExpansion");

	const float CurrentSyncTime = GetSyncWorldTimeSeconds();
	const float CurrentSimTime = CurrentSyncTime - ServerState.ExpansionStartTime;

	float StartSimTime = SimTime;
	float EndSimTime = CurrentSimTime;

	SimTime = CurrentSimTime;

	int32 TargetSpawnNum = 0;

	if (EndSimTime < ExpansionDuration)
	{
		float CurveValue = GetCurveValue(CurrentSimTime, ExpansionDuration, ExpansionCurve);
		TargetSpawnNum = FMath::FloorToInt(MaxVoxelNum * CurveValue);
	}
	else
	{
		EndSimTime = ExpansionDuration;
		TargetSpawnNum = MaxVoxelNum;
	}

	int32 SpawnNum = TargetSpawnNum - ActiveVoxelNum;

	if (!ExpansionHeap.IsEmpty() && SpawnNum > 0)
	{
		ProcessExpansion(SpawnNum, StartSimTime, EndSimTime);
	}

	if (CurrentSimTime >= ExpansionDuration + FadeInDuration)
	{
		if (HasAuthority())
		{
			ServerState.State = EIVSmokeVoxelVolumeState::Sustain;
			ServerState.SustainStartTime = GetSyncWorldTimeSeconds();

			HandleStateTransition(ServerState.State);
		}
	}
}

void AIVSmokeVoxelVolume::UpdateSustain()
{
	SCOPE_CYCLE_COUNTER(STAT_IVSmoke_UpdateSustain);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::AIVSmokeVoxelVolume::UpdateSustain");

	const float CurrentSyncTime = GetSyncWorldTimeSeconds();
	const float CurrentSimTime = CurrentSyncTime - ServerState.SustainStartTime;

	SimTime = CurrentSimTime;

	if (!bIsInfinite && CurrentSimTime >= SustainDuration)
	{
		if (HasAuthority())
		{
			ServerState.State = EIVSmokeVoxelVolumeState::Dissipation;
			ServerState.DissipationStartTime = GetSyncWorldTimeSeconds();

			HandleStateTransition(ServerState.State);
		}
	}
}

void AIVSmokeVoxelVolume::UpdateDissipation()
{
	SCOPE_CYCLE_COUNTER(STAT_IVSmoke_UpdateDissipation);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::AIVSmokeVoxelVolume::UpdateDissipation");

	const float CurrentSyncTime = GetSyncWorldTimeSeconds();
	const float CurrentSimTime = CurrentSyncTime - ServerState.DissipationStartTime;

	float StartSimTime = SimTime;
	float EndSimTime = CurrentSimTime;

	SimTime = CurrentSimTime;

	int32 TargetAliveNum = GeneratedVoxelIndices.Num();

	if (CurrentSimTime < DissipationDuration)
	{
		float CurveValue = 1.0f;
		if (DissipationCurve)
		{
			CurveValue = GetCurveValue(CurrentSimTime, DissipationDuration, DissipationCurve);
		}
		else
		{
			CurveValue = FMath::Max(0.0f, 1.0f - GetCurveValue(CurrentSimTime, DissipationDuration, DissipationCurve));
		}
		TargetAliveNum = FMath::FloorToInt(GeneratedVoxelIndices.Num() * CurveValue);
	}
	else
	{
		EndSimTime = DissipationDuration;
		TargetAliveNum = 0;
	}

	int32 RemoveNum = DissipationHeap.Num() - TargetAliveNum;

	if (RemoveNum > 0)
	{
		ProcessDissipation(RemoveNum, StartSimTime, EndSimTime);
	}

	if (CurrentSimTime >= DissipationDuration + FadeOutDuration)
	{
		SimTime = 0.0f;

		TryUpdateCollision(true);

		if (HasAuthority())
		{
			ServerState.State = EIVSmokeVoxelVolumeState::Finished;

			HandleStateTransition(ServerState.State);
		}
	}
}

void AIVSmokeVoxelVolume::ProcessExpansion(int32 SpawnNum, float StartSimTime, float EndSimTime)
{
	SCOPE_CYCLE_COUNTER(STAT_IVSmoke_ProcessExpansion);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::AIVSmokeVoxelVolume::ProcessExpansion");

	if (SpawnNum <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform ActorTrans = GetActorTransform();
	const FIntVector GridResolution = GetGridResolution();
	const FIntVector CenterOffset = GetCenterOffset();

	FVector InvRadii;
	InvRadii.X = 1.0f / FMath::Max(UE_KINDA_SMALL_NUMBER, Radii.X);
	InvRadii.Y = 1.0f / FMath::Max(UE_KINDA_SMALL_NUMBER, Radii.Y);
	InvRadii.Z = 1.0f / FMath::Max(UE_KINDA_SMALL_NUMBER, Radii.Z);

	const float InvSpawnNum = 1.0f / SpawnNum;
	int32 SpawnCount = 0;

	while (SpawnCount < SpawnNum && !ExpansionHeap.IsEmpty())
	{
		FIVSmokeVoxelNode CurrentNode;
		ExpansionHeap.HeapPop(CurrentNode);

		if (!VoxelCosts.IsValidIndex(CurrentNode.Index))
		{
			continue;
		}

		if (CurrentNode.Cost > VoxelCosts[CurrentNode.Index])
		{
			continue;
		}

		if (IsVoxelActive(CurrentNode.Index))
		{
			continue;
		}

		int32 ResolvedParentIndex = CurrentNode.ParentIndex;
		float ResolvedCost = CurrentNode.Cost;

		const FIntVector CurrentGrid = UIVSmokeGridLibrary::IndexToGrid(CurrentNode.Index, GridResolution);
		const FIntVector ParentGrid = UIVSmokeGridLibrary::IndexToGrid(CurrentNode.ParentIndex, GridResolution);
		const FVector CurrentLocal = UIVSmokeGridLibrary::GridToLocal(CurrentGrid, VoxelSize, CenterOffset);
		const FVector ParentLocal = UIVSmokeGridLibrary::GridToLocal(ParentGrid, VoxelSize, CenterOffset);
		const FVector CurrentWorld = ActorTrans.TransformPositionNoScale(CurrentLocal);
		const FVector ParentWorld = ActorTrans.TransformPositionNoScale(ParentLocal);

		bool bIsPenetrating = false;

		if (CurrentNode.ParentIndex != INDEX_NONE && VoxelCosts.IsValidIndex(CurrentNode.ParentIndex))
		{
			const int32 ManhattanDist = FMath::Abs(CurrentGrid.X - ParentGrid.X) +
										FMath::Abs(CurrentGrid.Y - ParentGrid.Y) +
										FMath::Abs(CurrentGrid.Z - ParentGrid.Z);
			if (ManhattanDist > 1)
			{
				if (IsConnectionBlocked(World, ParentWorld, CurrentWorld))
				{
					float BestRewireCost = FLT_MAX;
					int32 BestRewireParent = INDEX_NONE;

					for (const FIntVector& Direction : FloodFillDirections)
					{
						const FIntVector NeighborGrid = CurrentGrid + Direction;
						const int32 NeighborIndex = UIVSmokeGridLibrary::GridToIndex(NeighborGrid, GridResolution);

						if (!VoxelCosts.IsValidIndex(NeighborIndex))
						{
							continue;
						}

						if (IsVoxelActive(NeighborIndex))
						{
							if (VoxelPenetrationFlags[NeighborIndex])
							{
								continue;
							}

							const float Dist = CalculateWeightedDistance(NeighborIndex, CurrentNode.Index, InvRadii);
							const float NewCost = VoxelCosts[NeighborIndex] + Dist;

							if (NewCost < BestRewireCost)
							{
								BestRewireCost = NewCost;
								BestRewireParent = NeighborIndex;
							}
						}
					}

					if (BestRewireParent != INDEX_NONE)
					{
						ResolvedParentIndex = BestRewireParent;
						ResolvedCost = BestRewireCost;
						VoxelCosts[CurrentNode.Index] = ResolvedCost;

						const FIntVector NeighborGrid = UIVSmokeGridLibrary::IndexToGrid(BestRewireParent, GridResolution);
						const FVector NeighborLocal = UIVSmokeGridLibrary::GridToLocal(NeighborGrid, VoxelSize, CenterOffset);
						const FVector NeighborWorld = ActorTrans.TransformPosition(NeighborLocal);

						if (IsConnectionBlocked(World, CurrentWorld, NeighborWorld))
						{
							bIsPenetrating = true;
						}
					}
					else
					{
						continue;
					}
				}
			}
			else
			{
				if (IsConnectionBlocked(World, CurrentWorld, ParentWorld))
				{
					bIsPenetrating = true;
				}
			}
		}

		const float Alpha = SpawnCount * InvSpawnNum;
		const float BirthTime = ServerState.ExpansionStartTime + FMath::Lerp(StartSimTime, EndSimTime, Alpha);
		SetVoxelBirthTime(CurrentNode.Index, BirthTime);

		GeneratedVoxelIndices.Add(CurrentNode.Index);
		++SpawnCount;

		const float DissipationCost = ResolvedCost + RandomStream.FRandRange(0.0f, DissipationNoise);
		DissipationHeap.HeapPush({ CurrentNode.Index, INDEX_NONE, DissipationCost });

		if (GetActiveVoxelNum() >= MaxVoxelNum)
		{
			return;
		}

		if (bIsPenetrating)
		{
			VoxelPenetrationFlags[CurrentNode.Index] = true;

			continue;
		}

		for (const FIntVector& Direction : FloodFillDirections)
		{
			const FIntVector NextGrid = CurrentGrid + Direction;

			if (NextGrid.X < 0 || NextGrid.X >= GridResolution.X ||
				NextGrid.Y < 0 || NextGrid.Y >= GridResolution.Y ||
				NextGrid.Z < 0 || NextGrid.Z >= GridResolution.Z)
			{
				continue;
			}

			const int32 NextIndex = UIVSmokeGridLibrary::GridToIndex(NextGrid, GridResolution);

			if (!VoxelCosts.IsValidIndex(NextIndex))
			{
				continue;
			}

			if (ResolvedParentIndex != INDEX_NONE)
			{
				const float GrandparentDist = CalculateWeightedDistance(ResolvedParentIndex, NextIndex, InvRadii);
				const float Noise = RandomStream.FRandRange(0.0f, ExpansionNoise);
				const float NewCost = VoxelCosts[ResolvedParentIndex] + GrandparentDist + Noise;

				if (NewCost < VoxelCosts[NextIndex])
				{
					VoxelCosts[NextIndex] = NewCost;
					ExpansionHeap.HeapPush({NextIndex, ResolvedParentIndex, NewCost});
				}
			}
			else
			{
				const float CurrentDist = CalculateWeightedDistance(CurrentNode.Index, NextIndex, InvRadii);
				const float Noise = RandomStream.FRandRange(0.0f, ExpansionNoise);
				const float NewCost = ResolvedCost + CurrentDist + Noise;

				if (NewCost < VoxelCosts[NextIndex])
				{
					VoxelCosts[NextIndex] = NewCost;
					ExpansionHeap.HeapPush({NextIndex, CurrentNode.Index, NewCost});
				}
			}

		}
	}
}

void AIVSmokeVoxelVolume::ProcessDissipation(int32 RemoveNum, float StartSimTime, float EndSimTime)
{
	SCOPE_CYCLE_COUNTER(STAT_IVSmoke_ProcessDissipation);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::AIVSmokeVoxelVolume::ProcessDissipation");

	if (RemoveNum <= 0)
	{
		return;
	}

	float InvRemoveNum = 1.0f / RemoveNum;

	int32 RemoveCount = 0;
	while (RemoveCount < RemoveNum && !DissipationHeap.IsEmpty())
	{
		FIVSmokeVoxelNode CurrentNode;
		DissipationHeap.HeapPop(CurrentNode);

		float Alpha = RemoveCount * InvRemoveNum;
		float DeathTime = ServerState.DissipationStartTime + FMath::Lerp(StartSimTime, EndSimTime, Alpha);

		SetVoxelDeathTime(CurrentNode.Index, DeathTime);

		++RemoveCount;
	}
}

void AIVSmokeVoxelVolume::SetVoxelBirthTime(int32 Index, float BirthTime)
{
	if (!VoxelBirthTimes.IsValidIndex(Index))
	{
		return;
	}

	if (VoxelBirthTimes[Index] > 0.0f)
	{
		return;
	}

	float SafeBirthTime = FMath::Max(BirthTime, 0.001f);
	VoxelBirthTimes[Index] = SafeBirthTime;

	if (VoxelDeathTimes.IsValidIndex(Index))
	{
		VoxelDeathTimes[Index] = 0.0f;
	}

	FIntVector GridResolution = GetGridResolution();
	FIntVector CenterOffset = GetCenterOffset();

	UIVSmokeGridLibrary::SetVoxelBit(VoxelBits, Index, GridResolution, true);

	++ActiveVoxelNum;
	INC_DWORD_STAT(STAT_IVSmoke_CreatedVoxel);

	DirtyLevel = EIVSmokeDirtyLevel::Dirty;

	const FIntVector GridPos = UIVSmokeGridLibrary::IndexToGrid(Index, GridResolution);
	const FVector LocalPos = UIVSmokeGridLibrary::GridToLocal(GridPos, VoxelSize, CenterOffset);
	const FVector WorldPos = GetActorTransform().TransformPosition(LocalPos);
	VoxelWorldAABBMin = FVector::Min(WorldPos, VoxelWorldAABBMin);
	VoxelWorldAABBMax = FVector::Max(WorldPos, VoxelWorldAABBMax);
}

void AIVSmokeVoxelVolume::SetVoxelDeathTime(int32 Index, float DeathTime)
{
	if (!VoxelDeathTimes.IsValidIndex(Index))
	{
		return;
	}

	if (VoxelDeathTimes[Index] > 0.0f)
	{
		return;
	}

	const float SafeDeathTime = FMath::Max(DeathTime, 0.001f);
	VoxelDeathTimes[Index] = SafeDeathTime;

	FIntVector GridResolution = GetGridResolution();

	UIVSmokeGridLibrary::SetVoxelBit(VoxelBits, Index, GridResolution, false);

	--ActiveVoxelNum;
	INC_DWORD_STAT(STAT_IVSmoke_DestroyedVoxel)

	DirtyLevel = EIVSmokeDirtyLevel::Dirty;
}

float AIVSmokeVoxelVolume::CalculateWeightedDistance(int32 IndexA, int32 IndexB, const FVector& InvRadii) const
{
	if (!VoxelCosts.IsValidIndex(IndexA) || !VoxelCosts.IsValidIndex(IndexB))
	{
		return FLT_MAX;
	}

	const FIntVector GridResolution = GetGridResolution();
	const FIntVector CenterOffset = GetCenterOffset();

	const FIntVector GridA = UIVSmokeGridLibrary::IndexToGrid(IndexA, GridResolution);
	const FIntVector GridB = UIVSmokeGridLibrary::IndexToGrid(IndexB, GridResolution);

	const FVector DeltaGrid(GridB - GridA);
	const FVector DeltaLocal = DeltaGrid * VoxelSize;

	const float NormX = DeltaLocal.X * InvRadii.X;
	const float NormY = DeltaLocal.Y * InvRadii.Y;
	const float NormZ = DeltaLocal.Z * InvRadii.Z;

	return FMath::Sqrt(NormX * NormX + NormY * NormY + NormZ * NormZ);
}

#pragma endregion

//~==============================================================================
// Collision
#pragma region Collision

void AIVSmokeVoxelVolume::TryUpdateCollision(bool bForce)
{
	if (bIsFastForwarding)
	{
		return;
	}

	if (CollisionComponent)
	{
		CollisionComponent->TryUpdateCollision(
			VoxelBits,
			GetGridResolution(),
			VoxelSize,
			ActiveVoxelNum,
			GetSyncWorldTimeSeconds(),
			bForce
		);
	}
}

#pragma endregion

//~==============================================================================
// Data Access
#pragma region DataAccess

bool AIVSmokeVoxelVolume::ShouldRender() const
{
	// Respect Actor visibility
#if WITH_EDITOR
	// Editor: check both editor visibility (Outliner eye icon) and game visibility
	if (IsHiddenEd() || IsHidden())
	{
		return false;
	}

	// Editor preview mode: special handling
	if (bIsEditorPreviewing)
	{
		// If PIE is running, don't render editor preview volumes
		// This prevents conflicts between editor and PIE worlds
		if (GEditor && GEditor->IsPlayingSessionInEditor())
		{
			return false;
		}

		// Check debug settings
		if (!DebugSettings.bDebugEnabled || !DebugSettings.bRenderSmokeInPreview)
		{
			return false;
		}
	}
#else
	// Runtime: check game visibility only
	if (IsHidden())
	{
		return false;
	}
#endif

	const EIVSmokeVoxelVolumeState State = ServerState.State;
	return State == EIVSmokeVoxelVolumeState::Expansion
		|| State == EIVSmokeVoxelVolumeState::Sustain
		|| State == EIVSmokeVoxelVolumeState::Dissipation;
}

TObjectPtr<UIVSmokeHoleGeneratorComponent> AIVSmokeVoxelVolume::GetHoleGeneratorComponent()
{
	if (!IsValid(HoleGeneratorComponent))
	{
		HoleGeneratorComponent = FindComponentByClass<UIVSmokeHoleGeneratorComponent>();
	}

	return HoleGeneratorComponent;
}

TObjectPtr<UIVSmokeCollisionComponent> AIVSmokeVoxelVolume::GetCollisionComponent()
{
	if (!IsValid(CollisionComponent))
	{
		CollisionComponent = FindComponentByClass<UIVSmokeCollisionComponent>();
	}

	return CollisionComponent;
}

FTextureRHIRef AIVSmokeVoxelVolume::GetHoleTexture() const
{
	if (HoleGeneratorComponent)
	{
		return HoleGeneratorComponent->GetHoleTextureRHI();
	}
	return nullptr;
}

float AIVSmokeVoxelVolume::GetSyncWorldTimeSeconds() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	if (World->GetNetMode() == NM_Client)
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
	}

	return World->GetTimeSeconds();
}

#pragma endregion

//~==============================================================================
// Debug
#pragma region Debug

void AIVSmokeVoxelVolume::StartPreviewSimulation()
{
#if WITH_EDITOR
	if (!DebugSettings.bDebugEnabled)
	{
		return;
	}

	bIsEditorPreviewing = true;

	StartSimulationInternal();
#endif
}

void AIVSmokeVoxelVolume::StopPreviewSimulation()
{
#if WITH_EDITOR
	bIsEditorPreviewing = false;

	ResetSimulationInternal();

	if (DebugMeshComponent)
	{
		DebugMeshComponent->ClearInstances();
	}
#endif
}

void AIVSmokeVoxelVolume::DrawDebugVisualization() const
{
#if WITH_EDITOR
	if (!DebugSettings.bDebugEnabled)
	{
		return;
	}

	DrawDebugVolumeBounds();
	DrawDebugVoxelWireframes();
	DrawDebugVoxelMeshes();
	DrawDebugStatusText();

	if (CollisionComponent)
	{
		CollisionComponent->DrawDebugVisualization();
	}
#endif
}

void AIVSmokeVoxelVolume::DrawDebugVolumeBounds() const
{
#if WITH_EDITOR
	if (!DebugSettings.bShowVolumeBounds)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FIntVector GridResolution = GetGridResolution();

	FVector TotalSize;
	TotalSize.X = GridResolution.X * VoxelSize;
	TotalSize.Y = GridResolution.Y * VoxelSize;
	TotalSize.Z = GridResolution.Z * VoxelSize;

	FVector HalfExtent = TotalSize * 0.5f;

	FVector CenterPos = GetActorLocation();
	FQuat Rotation = GetActorRotation().Quaternion();

	DrawDebugBox(
		World,
		CenterPos,
		HalfExtent,
		Rotation,
		FColor(100, 255, 100),
		false,
		-1.0f,
		0,
		2.0f
	);
#endif
}

void AIVSmokeVoxelVolume::DrawDebugVoxelWireframes() const
{
#if WITH_EDITOR
	if (!DebugSettings.bShowVoxelWireframe || GeneratedVoxelIndices.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTransform ActorTrans = GetActorTransform();
	int32 VoxelNum = GeneratedVoxelIndices.Num();
	int32 MaxVisibleIndex = FMath::Clamp(VoxelNum * DebugSettings.VisibleStepCountPercent / 100.0f, 0, VoxelNum);

	const FVector HalfVoxelSize(VoxelSize * 0.5f);
	for (int32 i = 0; i < MaxVisibleIndex; ++i)
	{
		int32 VoxelIndex = GeneratedVoxelIndices[i];
		if (!IsVoxelActive(VoxelIndex))
		{
			continue;
		}

		FIntVector GridResolution = GetGridResolution();
		FIntVector CenterOffset = GetCenterOffset();

		FIntVector GridPos = UIVSmokeGridLibrary::IndexToGrid(VoxelIndex, GridResolution);
		float NormHeight = static_cast<float>(GridPos.Z) / static_cast<float>(GridResolution.Z);
		if (NormHeight > DebugSettings.SliceHeight)
		{
			continue;
		}

		FVector LocalPos = UIVSmokeGridLibrary::GridToLocal(GridPos, VoxelSize, CenterOffset);
		FVector WorldPos = ActorTrans.TransformPosition(LocalPos);

		DrawDebugBox(
			World,
			WorldPos,
			HalfVoxelSize,
			ActorTrans.GetRotation(),
			DebugSettings.DebugWireframeColor,
			false, -1.0f, 0, 1.5f
		);
	}
#endif
}

void AIVSmokeVoxelVolume::DrawDebugVoxelMeshes() const
{
#if WITH_EDITOR
	if (!DebugMeshComponent)
	{
		return;
	}

	if (!DebugSettings.bShowVoxelMesh || GeneratedVoxelIndices.IsEmpty())
	{
		DebugMeshComponent->ClearInstances();
		return;
	}

	if (DebugVoxelMesh && DebugMeshComponent->GetStaticMesh() != DebugVoxelMesh)
	{
		DebugMeshComponent->SetStaticMesh(DebugVoxelMesh);
	}
	if (DebugVoxelMaterial && DebugMeshComponent->GetMaterial(0) != DebugVoxelMaterial)
	{
		DebugMeshComponent->SetMaterial(0, DebugVoxelMaterial);
	}

	DebugMeshComponent->ClearInstances();

	int32 VoxelNum = GeneratedVoxelIndices.Num();
	int32 MaxVisibleIndex = FMath::Clamp(static_cast<int32>(VoxelNum * DebugSettings.VisibleStepCountPercent / 100.0f), 0, VoxelNum);

	TArray<FTransform> InstanceTransforms;
	InstanceTransforms.Reserve(MaxVisibleIndex);

	TArray<float> InstanceCustomData;
	InstanceCustomData.Reserve(MaxVisibleIndex);

	const FVector Scale3D(VoxelSize / 100.0f * 0.98f);

	for (int32 i = 0; i < MaxVisibleIndex; ++i)
	{
		int32 VoxelIndex = GeneratedVoxelIndices[i];

		if (!IsVoxelActive(VoxelIndex))
		{
			continue;
		}

		FIntVector GridResolution = GetGridResolution();
		FIntVector CenterOffset = GetCenterOffset();

		FIntVector GridPos = UIVSmokeGridLibrary::IndexToGrid(VoxelIndex, GridResolution);

		float NormHeight = static_cast<float>(GridPos.Z) / static_cast<float>(GridResolution.Z);
		if (NormHeight > DebugSettings.SliceHeight)
		{
			continue;
		}

		FVector LocalPos = UIVSmokeGridLibrary::GridToLocal(GridPos, VoxelSize, CenterOffset);

		FTransform InstanceTrans;
		InstanceTrans.SetLocation(LocalPos);
		InstanceTrans.SetRotation(FQuat::Identity);
		InstanceTrans.SetScale3D(Scale3D);

		InstanceTransforms.Add(InstanceTrans);

		float DataValue = 0.0f;
		if (DebugSettings.ViewMode == EIVSmokeDebugViewMode::Heatmap)
		{
			DataValue = (VoxelNum > 1) ? static_cast<float>(i) / static_cast<float>(VoxelNum - 1) : 0.0f;
		}
		InstanceCustomData.Add(DataValue);
	}

	if (InstanceTransforms.Num() > 0)
	{
		DebugMeshComponent->AddInstances(InstanceTransforms, false, false);

		int32 InstanceNum = InstanceTransforms.Num();
		for (int32 i = 0; i < InstanceNum; ++i)
		{
			bool bIsLast = (i == InstanceNum - 1);
			DebugMeshComponent->SetCustomDataValue(i, 0, InstanceCustomData[i], bIsLast);
		}
	}
#endif
}

void AIVSmokeVoxelVolume::DrawDebugStatusText() const
{
#if WITH_EDITOR
	if (!DebugSettings.bDebugEnabled || !DebugSettings.bShowStatusText)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FString StateStr;
	switch (ServerState.State)
	{
	case EIVSmokeVoxelVolumeState::Idle:		StateStr = TEXT("Idle"); break;
	case EIVSmokeVoxelVolumeState::Expansion:	StateStr = TEXT("Expansion"); break;
	case EIVSmokeVoxelVolumeState::Sustain:		StateStr = TEXT("Sustain"); break;
	case EIVSmokeVoxelVolumeState::Dissipation:	StateStr = TEXT("Dissipation"); break;
	case EIVSmokeVoxelVolumeState::Finished:	StateStr = TEXT("Finished"); break;
	default:									StateStr = TEXT("Unknown"); break;
	}

	float Percent = MaxVoxelNum > 0 ? (static_cast<float>(ActiveVoxelNum) / MaxVoxelNum * 100.0f) : 0.0f;

	FString DebugMsg = FString::Printf(
		TEXT("State: %s\nSeed: %d\nTime: %.2fs\nVoxels: %d / %d (%.1f%%)\nHeap: %d\nChecksum: %u"),
		*StateStr,
		ServerState.RandomSeed,
		SimTime,
		ActiveVoxelNum,
		MaxVoxelNum,
		Percent,
		ExpansionHeap.Num(),
		CalculateSimulationChecksum()
	);

	FIntVector GridResolution = GetGridResolution();

	FVector TextPos = GetActorLocation();
	TextPos.Z += GridResolution.Z * VoxelSize * 0.25f;

	DrawDebugString(World, TextPos, DebugMsg, nullptr, FColor::White, 0.0f, true, 1.2f);
#endif
}

uint32 AIVSmokeVoxelVolume::CalculateSimulationChecksum() const
{
	uint32 Checksum = 0;

	Checksum = FCrc::MemCrc32(&ActiveVoxelNum, sizeof(int32), Checksum);

	int32 StateInt = (int32)ServerState.State;
	Checksum = FCrc::MemCrc32(&StateInt, sizeof(int32), Checksum);

	if (VoxelBits.Num() > 0)
	{
		Checksum = FCrc::MemCrc32(VoxelBits.GetData(), VoxelBits.Num() * sizeof(uint64), Checksum);
	}

	return Checksum;
}

#pragma endregion
