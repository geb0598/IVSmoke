// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeHolePreset.h"

static TMap<uint8, TWeakObjectPtr<UIVSmokeHolePreset>> GHolePresetRegistry;

void UIVSmokeHolePreset::PostLoad()
{
	Super::PostLoad();

	RegisterToGlobalRegistry();
}

void UIVSmokeHolePreset::BeginDestroy()
{
	UnregisterFromGlobalRegistry();

	Super::BeginDestroy();
}

void UIVSmokeHolePreset::RegisterToGlobalRegistry()
{
	uint8 ID = static_cast<uint8>(GetTypeHash(GetPathName()));
	const uint8 StartID = ID;
	TObjectPtr<UIVSmokeHolePreset> ToInsert = this;

	while (TObjectPtr<UIVSmokeHolePreset> Existing = GHolePresetRegistry.FindRef(ID).Get())
	{
		if (Existing == ToInsert)
		{
			return;
		}

		if (ToInsert->GetPathName() < Existing->GetPathName())
		{
			GHolePresetRegistry.Remove(ID);
			ToInsert->CachedID = ID;
			GHolePresetRegistry.Add(ID, ToInsert);
			ToInsert = Existing;
		}

		ID++;

		if (ID == StartID)
		{
			ensureMsgf(false, TEXT("[UIVSmokeHolePreset] Registry full: %s"), *ToInsert->GetName());
			return;
		}
	}

	ToInsert->CachedID = ID;
	GHolePresetRegistry.Add(ID, ToInsert);
}

void UIVSmokeHolePreset::UnregisterFromGlobalRegistry()
{
	GHolePresetRegistry.Remove(CachedID);
}

TObjectPtr<UIVSmokeHolePreset> UIVSmokeHolePreset::FindByID(const uint8 InPresetID)
{
	return GHolePresetRegistry.FindRef(InPresetID).Get();
}

float UIVSmokeHolePreset::GetFloatValue(const TObjectPtr<UCurveFloat> Curve, const float X)
{
	if (Curve.Get())
	{
		return Curve.Get()->GetFloatValue(X);
	}
	return 0;
}
