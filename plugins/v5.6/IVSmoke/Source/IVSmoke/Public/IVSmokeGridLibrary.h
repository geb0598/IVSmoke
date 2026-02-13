// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IVSmokeGridLibrary.generated.h"

/**
 * Utility library for smoke grid calculations and voxel bit operations.
 */
UCLASS()
class IVSMOKE_API UIVSmokeGridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Invalid grid pos (-1, -1, -1) */
	static const FIntVector InvalidGridPos;

	/**
	 * 3D grid coordinate to 1D index.
	 *
	 * @param GridPos			3D grid coordinate.
	 * @param Resolution		3D grid resolution.
	 * @return					1D flattened index
	 */
	UFUNCTION(BlueprintPure, Category = "IVSmoke | Math")
	static FORCEINLINE int32 GridToIndex(const FIntVector& GridPos, const FIntVector& Resolution)
	{
		return GridPos.X + (GridPos.Y * Resolution.X) + (GridPos.Z * Resolution.X * Resolution.Y);
	}

	/**
	 * 1D flattened index to 3D grid coordinate.
	 *
	 * @param Index				1D flattened index.
	 * @param Resolution		3D grid resolution.
	 * @return					3D grid coordinate.
	 */
	UFUNCTION(BlueprintPure, Category = "IVSmoke | Math")
	static FORCEINLINE FIntVector IndexToGrid(int32 Index, const FIntVector& Resolution)
	{
		if (Resolution.X <= 0 || Resolution.Y <= 0 || Resolution.Z <= 0)
		{
			return FIntVector::ZeroValue;
		}

		const int32 BaseArea = Resolution.X * Resolution.Y;
		const int32 Z = Index / BaseArea;
		const int32 Remainder = Index % BaseArea;
		const int32 Y = Remainder / Resolution.X;
		const int32 X = Remainder % Resolution.X;

		return FIntVector(X, Y, Z);
	}

	/**
	 * Converts 3D grid coordinate to local space position.
	 *
	 * @param GridPos			3D grid coordinate.
	 * @param VoxelSize			Size of each voxel.
	 * @param CenterOffset		Grid center offset.
	 * @return					Local space position.
	 */
	UFUNCTION(BlueprintPure, Category = "IVSmoke | Math")
	static FORCEINLINE FVector GridToLocal(const FIntVector& GridPos, float VoxelSize, const FIntVector& CenterOffset)
	{
		return FVector(
			(GridPos.X - CenterOffset.X) * VoxelSize,
			(GridPos.Y - CenterOffset.Y) * VoxelSize,
			(GridPos.Z - CenterOffset.Z) * VoxelSize
		);
	}

	/**
	 * Converts local space position to 3D grid coordinate.
	 *
	 * @param LocalPos			Local space position.
	 * @param VoxelSize			Size of each voxel.
	 * @param CenterOffset		Grid center offset.
	 * @param Resolution		3D grid resolution.
	 * @return					3D grid coordinate, or InvalidGridPos if out of bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "IVSmoke | Math")
	static FORCEINLINE FIntVector LocalToGrid(const FVector& LocalPos, float VoxelSize, const FIntVector& CenterOffset, const FIntVector& Resolution)
	{
		if (VoxelSize <= UE_SMALL_NUMBER)
		{
			return InvalidGridPos;
		}

		const int32 X = FMath::RoundToInt(LocalPos.X / VoxelSize) + CenterOffset.X;
		const int32 Y = FMath::RoundToInt(LocalPos.Y / VoxelSize) + CenterOffset.Y;
		const int32 Z = FMath::RoundToInt(LocalPos.Z / VoxelSize) + CenterOffset.Z;

		if (X >= 0 && X < Resolution.X &&
			Y >= 0 && Y < Resolution.Y &&
			Z >= 0 && Z < Resolution.Z)
		{
			return FIntVector(X, Y, Z);
		}

		return InvalidGridPos;
	}

	//~==============================================================================
	// Bitmask Helpers

	/**
	 * Converts 3D grid coordinate to voxel bit index.
	 *
	 * @param GridPos			3D grid coordinate.
	 * @param Resolution		3D grid resolution.
	 * @return					Voxel bit index.
	 */
	static FORCEINLINE int32 GridToVoxelBitIndex(const FIntVector& GridPos, const FIntVector& Resolution)
	{
		return GridToVoxelBitIndex(GridPos.Y, GridPos.Z, Resolution.Y);
	}

	/**
	 * Converts Y and Z coordinates to voxel bit index.
	 *
	 * @param Y					Y coordinate.
	 * @param Z					Z coordinate.
	 * @param ResolutionY		Y resolution.
	 * @return					Voxel bit index.
	 */
	static FORCEINLINE int32 GridToVoxelBitIndex(int32 Y, int32 Z, int32 ResolutionY)
	{
		return Y + (Z * ResolutionY);
	}

	/**
	 * Checks if a voxel occupancy bit is set at the given 3D grid position.
	 * Internally, X is stored as a bit index in a uint64, while Y and Z are mapped to the array index.
	 *
	 * @param VoxelBitArray     Bit-packed voxel occupancy array (uint64 per YZ slice).
	 * @param GridPos           3D grid coordinate.
	 * @param Resolution        3D grid resolution (each axis < 64).
	 * @return                  True if the voxel bit is set, false otherwise.
	 */
	static FORCEINLINE bool IsVoxelBitSet(const TArray<uint64>& VoxelBitArray, const FIntVector& GridPos, const FIntVector& Resolution)
	{
		// Resolution must be less than 64 for each axis to fit in uint64 bitfield
		check(Resolution.X >= 0 && Resolution.X < 64 &&
			  Resolution.Y >= 0 && Resolution.Y < 64 &&
			  Resolution.Z >= 0 && Resolution.Z < 64);

		const int32 Index = GridToVoxelBitIndex(GridPos, Resolution);

		if (!VoxelBitArray.IsValidIndex(Index))
		{
			return false;
		}

		return VoxelBitArray[Index] & (1ULL << GridPos.X);
	}

	/**
	 * Sets a voxel bit value at the given 1D index.
	 *
	 * @param VoxelBitArray		Bit-packed voxel occupancy array (uint64 per YZ slice).
	 * @param Index				1D flattened index.
	 * @param Resolution		3D grid resolution (each axis < 64).
	 * @param bValue			Value to set (true or false).
	 */
	static FORCEINLINE void SetVoxelBit(TArray<uint64>& VoxelBitArray, int32 Index, const FIntVector& Resolution, bool bValue)
	{
		const FIntVector GridPos = IndexToGrid(Index, Resolution);
		SetVoxelBit(VoxelBitArray, GridPos, Resolution, bValue);
	}

	/**
	 * Sets a voxel bit value at the given 3D grid position.
	 * Internally, X is stored as a bit index in a uint64, while Y and Z are mapped to the array index.
	 *
	 * @param VoxelBitArray		Bit-packed voxel occupancy array (uint64 per YZ slice).
	 * @param GridPos			3D grid coordinate.
	 * @param Resolution		3D grid resolution (each axis < 64).
	 * @param bValue			Value to set (true or false).
	 */
	static FORCEINLINE void SetVoxelBit(TArray<uint64>& VoxelBitArray, const FIntVector& GridPos, const FIntVector& Resolution, bool bValue)
	{
		// Resolution must be less than 64 for each axis to fit in uint64 bitfield
		check(Resolution.X >= 0 && Resolution.X < 64 &&
			  Resolution.Y >= 0 && Resolution.Y < 64 &&
			  Resolution.Z >= 0 && Resolution.Z < 64);

		const int32 Index = GridToVoxelBitIndex(GridPos, Resolution);

		if (!VoxelBitArray.IsValidIndex(Index))
		{
			return;
		}

		if (bValue)
		{
			VoxelBitArray[Index] |= (1ULL << GridPos.X);
		}
		else
		{
			VoxelBitArray[Index] &= ~(1ULL << GridPos.X);
		}
	}

	/**
	 * Toggles a voxel bit value at the given 1D index.
	 *
	 * @param VoxelBitArray		Bit-packed voxel occupancy array (uint64 per YZ slice).
	 * @param Index				1D flattened index.
	 * @param Resolution		3D grid resolution (each axis < 64).
	 */
	static FORCEINLINE void ToggleVoxelBit(TArray<uint64>& VoxelBitArray, int32 Index, const FIntVector& Resolution)
	{
		const FIntVector GridPos = IndexToGrid(Index, Resolution);
		ToggleVoxelBit(VoxelBitArray, GridPos, Resolution);
	}

	/**
	 * Toggles a voxel bit value at the given 3D grid position.
	 * Internally, X is stored as a bit index in a uint64, while Y and Z are mapped to the array index.
	 *
	 * @param VoxelBitArray		Bit-packed voxel occupancy array (uint64 per YZ slice).
	 * @param GridPos			3D grid coordinate.
	 * @param Resolution		3D grid resolution (each axis < 64).
	 */
	static FORCEINLINE void ToggleVoxelBit(TArray<uint64>& VoxelBitArray, const FIntVector& GridPos, const FIntVector& Resolution)
	{
		// Resolution must be less than 64 for each axis to fit in uint64 bitfield
		check(Resolution.X >= 0 && Resolution.X < 64 &&
			  Resolution.Y >= 0 && Resolution.Y < 64 &&
			  Resolution.Z >= 0 && Resolution.Z < 64);

		const int32 Index = GridToVoxelBitIndex(GridPos, Resolution);

		if (!VoxelBitArray.IsValidIndex(Index))
		{
			return;
		}

		VoxelBitArray[Index] ^= (1ULL << GridPos.X);
	}
};
