// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeCollisionComponent.h"

#include "IVSmoke.h"
#include "IVSmokeGridLibrary.h"
#include "PhysicsEngine/BodySetup.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DECLARE_CYCLE_STAT(TEXT("Update Collision"), STAT_IVSmoke_UpdateCollision, STATGROUP_IVSmoke)
DECLARE_CYCLE_STAT(TEXT("Update Collision With Octree"), STAT_IVSmoke_UpdateCollisionWithOctree, STATGROUP_IVSmoke)
DECLARE_CYCLE_STAT(TEXT("Rebuild Physics Geometry"), STAT_IVSmoke_RebuildPhysicsGeometry, STATGROUP_IVSmoke)

//~==============================================================================
// Component Lifecycle
#pragma region Lifecycle

UIVSmokeCollisionComponent::UIVSmokeCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetGenerateOverlapEvents(false);

	Mobility = EComponentMobility::Movable;

	BodyInstance.SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);

	BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	BodyInstance.SetObjectType(ECC_WorldDynamic);

	BodyInstance.SetResponseToAllChannels(ECR_Ignore);

	BodyInstance.SetResponseToChannel(ECC_Visibility, ECR_Block);
}

UBodySetup* UIVSmokeCollisionComponent::GetBodySetup()
{
#if WITH_EDITOR
	// Prevent BodyInstanceCustomization crash when editor-world actor is selected during PIE
	if (GEditor && GEditor->IsPlaySessionInProgress())
	{
		const UWorld* World = GetWorld();
		if (World && World->WorldType == EWorldType::Editor)
		{
			return nullptr;
		}
	}
#endif

	if (!VoxelBodySetup)
	{
		VoxelBodySetup = NewObject<UBodySetup>(this, NAME_None, RF_Transient);
		VoxelBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		VoxelBodySetup->bNeverNeedsCookedCollisionData = true;
	}
	return VoxelBodySetup;
}

void UIVSmokeCollisionComponent::OnCreatePhysicsState()
{
	GetBodySetup();

	Super::OnCreatePhysicsState();
}

void UIVSmokeCollisionComponent::TryUpdateCollision(const TArray<uint64>& VoxelBitArray, const FIntVector& GridResolution, float VoxelSize, int32 ActiveVoxelNum, float SyncTime, bool bForce)
{
	if (GetCollisionEnabled() == ECollisionEnabled::NoCollision)
	{
		if (VoxelBodySetup && VoxelBodySetup->AggGeom.BoxElems.Num() > 0)
		{
			ResetCollision();
		}
		return;
	}

	if (!bForce)
	{
		if (LastSyncTime > 0.0f && (SyncTime - LastSyncTime) < MinCollisionUpdateInterval)
		{
			return;
		}

		int32 Diff = FMath::Abs(ActiveVoxelNum - LastActiveVoxelNum);

		if (Diff < MinCollisionUpdateVoxelNum)
		{
			return;
		}
	}

	UpdateCollision(VoxelBitArray, GridResolution, VoxelSize);

	LastSyncTime = SyncTime;
	LastActiveVoxelNum = ActiveVoxelNum;
}

#pragma endregion

//~==============================================================================
// Collision Management
#pragma region Collision

void UIVSmokeCollisionComponent::UpdateCollision(const TArray<uint64>& VoxelBitArray, const FIntVector& GridResolution, float VoxelSize)
{
	SCOPE_CYCLE_COUNTER(STAT_IVSmoke_UpdateCollision);

	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT("IVSmoke::UIVSmokeCollisionComponent::UpdateCollision");

	UBodySetup* BodySetup = GetBodySetup();
	if (!BodySetup)
	{
		return;
	}

	BodySetup->AggGeom.EmptyElements();

	TArray<uint64> TempVoxelBitArray = VoxelBitArray;

	const int32 ResolutionY = GridResolution.Y;
	const int32 ResolutionZ = GridResolution.Z;

	const FIntVector CenterOffset = GridResolution / 2;

	const float VoxelExtent = VoxelSize * 0.5f;

	for (int32 Z = 0; Z < ResolutionZ; ++Z)
	{
		for (int32 Y = 0; Y < ResolutionY; ++Y)
		{
			const int32 Index = UIVSmokeGridLibrary::GridToVoxelBitIndex(Y, Z, ResolutionY);

			uint64& CurrentRow = TempVoxelBitArray[Index];
			while (CurrentRow)
			{
				const int32 BeginX = FMath::CountTrailingZeros64(CurrentRow);

				const uint64 Shifted = CurrentRow >> BeginX;

				const int32 Width = (Shifted == MAX_uint64) ? (64 - BeginX) : FMath::CountTrailingZeros64(~Shifted);

				const uint64 Mask = (Width == 64) ? MAX_uint64 : ((1ULL << Width) - 1ULL) << BeginX;

				int32 Height = 1;
				for (int32 NextY = Y + 1; NextY < ResolutionY; ++NextY)
				{
					const int32 NextIndex = UIVSmokeGridLibrary::GridToVoxelBitIndex(NextY, Z, ResolutionY);

					const uint64& NextRow = TempVoxelBitArray[NextIndex];
					if ((NextRow & Mask) == Mask)
					{
						++Height;
					}
					else
					{
						break;
					}
				}

				int32 Depth = 1;
				for (int32 NextZ = Z + 1; NextZ < ResolutionZ; ++NextZ)
				{
					bool bCanExpand = true;
					for (int32 H = 0; H < Height; ++H)
					{
						const int32 NextIndex = UIVSmokeGridLibrary::GridToVoxelBitIndex(Y + H, NextZ, ResolutionY);

						const uint64& NextRow = TempVoxelBitArray[NextIndex];
						if ((NextRow & Mask) != Mask)
						{
							bCanExpand = false;
							break;
						}
					}

					if (bCanExpand)
					{
						++Depth;
					}
					else
					{
						break;
					}
				}

				for (int32 D = 0; D < Depth; ++D)
				{
					for (int32 H = 0; H < Height; ++H)
					{
						const int32 NextIndex = UIVSmokeGridLibrary::GridToVoxelBitIndex(Y + H, Z + D, ResolutionY);

						TempVoxelBitArray[NextIndex] &= ~Mask;
					}
				}

				FKBoxElem Box;

				FIntVector BeginGridPos(BeginX, Y, Z);
				FVector BeginVoxelCenter = UIVSmokeGridLibrary::GridToLocal(BeginGridPos, VoxelSize, CenterOffset);
				FVector CenterShift((Width - 1) * VoxelExtent, (Height - 1) * VoxelExtent, (Depth - 1) * VoxelExtent);
				Box.Center = BeginVoxelCenter + CenterShift;

				Box.X = Width * VoxelSize;
				Box.Y = Height * VoxelSize;
				Box.Z = Depth * VoxelSize;
				Box.Rotation = FRotator::ZeroRotator;

				BodySetup->AggGeom.BoxElems.Add(Box);
			}
		}
	}

	FinalizePhysicsUpdate();
}

void UIVSmokeCollisionComponent::ResetCollision()
{
	if (VoxelBodySetup)
	{
		VoxelBodySetup->AggGeom.EmptyElements();
		VoxelBodySetup->InvalidatePhysicsData();
		VoxelBodySetup->CreatePhysicsMeshes();
	}

	RecreatePhysicsState();
}

void UIVSmokeCollisionComponent::FinalizePhysicsUpdate()
{
	if (!VoxelBodySetup)
	{
		return;
	}

	VoxelBodySetup->InvalidatePhysicsData();
	VoxelBodySetup->CreatePhysicsMeshes();

	RecreatePhysicsState();
}

void UIVSmokeCollisionComponent::DrawDebugVisualization() const
{
#if WITH_EDITOR
	if (!bDebugEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!VoxelBodySetup)
	{
		return;
	}

	FTransform ComponentTrans = GetComponentTransform();

	const float GapScale = 0.95f;

	for (const FKBoxElem& Box : VoxelBodySetup->AggGeom.BoxElems)
	{
		FVector WorldCenter = ComponentTrans.TransformPosition(Box.Center);

		FVector Extent(Box.X * 0.5f * GapScale, Box.Y * 0.5f * GapScale, Box.Z * 0.5f * GapScale);

		FQuat WorldRotation = ComponentTrans.GetRotation() * Box.Rotation.Quaternion();

		uint32 Hash = GetTypeHash(Box.Center);
		FRandomStream StableRNG(Hash);

		float Hue = StableRNG.FRandRange(0.0f, 360.0f);
		FLinearColor BoxColor = FLinearColor::MakeFromHSV8(Hue, 200, 255);

		DrawDebugBox(
			World,
			WorldCenter,
			Extent,
			WorldRotation,
			BoxColor.ToFColor(true),
			false, -1.0f, 0, 1.5f
		);
	}

	FVector TextPos = GetComponentLocation() + FVector(0, 0, 50.0f);
	FString DebugMsg = FString::Printf(TEXT("Collision Boxes: %d"), VoxelBodySetup->AggGeom.BoxElems.Num());
	DrawDebugString(World, TextPos, DebugMsg, nullptr, FColor::White, 0.0f, true);
#endif
}

#pragma endregion
