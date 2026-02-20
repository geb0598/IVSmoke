// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeHoleData.h"
#include "IVSmokeHolePreset.h"
#include "IVSmokeHoleGeneratorComponent.h"

void FIVSmokeHoleData::PostReplicatedAdd(const FIVSmokeHoleArray& InArray)
{
	if (InArray.OwnerComponent)
	{
		InArray.OwnerComponent->MarkHoleTextureDirty();
	}
}

void FIVSmokeHoleData::PostReplicatedChange(const FIVSmokeHoleArray& InArray)
{
	if (InArray.OwnerComponent)
	{
		InArray.OwnerComponent->MarkHoleTextureDirty();
	}
}

void FIVSmokeHoleData::PreReplicatedRemove(const FIVSmokeHoleArray& InArray)
{
	if (InArray.OwnerComponent)
	{
		InArray.OwnerComponent->MarkHoleTextureDirty();
	}
}

void FIVSmokeHoleArray::Empty()
{
	Items.Empty();
	MarkArrayDirty();
}

TArray<FIVSmokeHoleGPU> FIVSmokeHoleArray::GetHoleGPUData(const float CurrentServerTime) const
{
	TArray<FIVSmokeHoleGPU> GPUBuffer;
	TArray<FIVSmokeHoleGPU> BulletBuffer;
	TArray<FIVSmokeHoleGPU> GrenadeBuffer;
	TArray<FIVSmokeHoleGPU> DynamicObjectBuffer;

	BulletBuffer.Reserve(FMath::Max(Num(), 1));
	GrenadeBuffer.Reserve(FMath::Max(Num(), 1));
	DynamicObjectBuffer.Reserve(FMath::Max(Num(), 1));
	GPUBuffer.Reserve(FMath::Max(Num(), 1));

	for (const FIVSmokeHoleData& Hole : Items)
	{
		TObjectPtr<UIVSmokeHolePreset> Preset = UIVSmokeHolePreset::FindByID(Hole.PresetID);
		if (!Preset)
		{
			continue;
		}

		FIVSmokeHoleGPU GPUHole = FIVSmokeHoleGPU(Hole, *Preset.Get(), CurrentServerTime);

		if (Preset->HoleType == EIVSmokeHoleType::Penetration)
		{
			BulletBuffer.Add(GPUHole);
		}
		else if (Preset->HoleType == EIVSmokeHoleType::Explosion)
		{
			GrenadeBuffer.Add(GPUHole);
		}
		else if (Preset->HoleType == EIVSmokeHoleType::Dynamic)
		{
			DynamicObjectBuffer.Add(GPUHole);
		}
	}

	GPUBuffer.Append(GrenadeBuffer);
	GPUBuffer.Append(BulletBuffer);
	GPUBuffer.Append(DynamicObjectBuffer);

	if (GPUBuffer.Num() == 0)
	{
		GPUBuffer.AddDefaulted(1);
	}

	return GPUBuffer;
}

FIVSmokeHoleGPU::FIVSmokeHoleGPU(const FIVSmokeHoleData& DynamicHoleData, const UIVSmokeHolePreset& Preset, const float CurrentServerTime)
{
	Position = FVector3f(DynamicHoleData.Position);
	EndPosition = FVector3f(DynamicHoleData.EndPosition);

	HoleType = static_cast<int32>(Preset.HoleType);
	Radius = Preset.Radius;
	Duration = Preset.Duration;
	if (Duration == 0)
	{
		return;
	}
	Softness = Preset.Softness;
	ExpansionDuration = Preset.ExpansionDuration;

	//SetTime
	float RemainingTime = DynamicHoleData.ExpirationServerTime - CurrentServerTime;
	CurLifeTime = Duration - RemainingTime;
	float NormalizedTime = FMath::Clamp(CurLifeTime / Duration, 0.0f, 1.0f);

	switch (Preset.HoleType)
	{
	case EIVSmokeHoleType::Explosion:
	{
		float ExpansionNormalizedTime = FMath::Clamp(CurLifeTime / Preset.ExpansionDuration, 0.0f, 1.0f);
		float ShrinkNormalizedTime = FMath::Clamp((CurLifeTime - Preset.ExpansionDuration) / (Preset.Duration - Preset.ExpansionDuration), 0.0f, 1.0f);

		CurExpansionFadeRangeOverTime = Preset.ExpansionFadeRangeCurveOverTime ? UIVSmokeHolePreset::GetFloatValue(Preset.ExpansionFadeRangeCurveOverTime, ExpansionNormalizedTime) : ExpansionNormalizedTime;
		CurShrinkFadeRangeOverTime = Preset.ShrinkFadeRangeCurveOverTime ? UIVSmokeHolePreset::GetFloatValue(Preset.ShrinkFadeRangeCurveOverTime, ShrinkNormalizedTime) : 1 - ShrinkNormalizedTime;
		DistortionExpOverTime = Preset.DistortionExpOverTime;
		DistortionDistance = Preset.DistortionDistance;
		break;
	}
	case EIVSmokeHoleType::Penetration:
		EndRadius = Preset.EndRadius;
		break;
	case EIVSmokeHoleType::Dynamic:
		Extent = Preset.Extent;
		break;
	}
}
