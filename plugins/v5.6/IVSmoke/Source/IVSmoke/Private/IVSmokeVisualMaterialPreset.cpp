// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeVisualMaterialPreset.h"
#include "IVSmoke.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

#if WITH_EDITOR
void UIVSmokeVisualMaterialPreset::PostEditChangeProperty(FPropertyChangedEvent& E)
{
	Super::PostEditChangeProperty(E);

	const FName PropName = E.Property ? E.Property->GetFName() : NAME_None;

	if (PropName == GET_MEMBER_NAME_CHECKED(UIVSmokeVisualMaterialPreset, SmokeVisualMaterial))
	{
		if (SmokeVisualMaterial)
		{
			if (UMaterial* M = SmokeVisualMaterial->GetMaterial())
			{
				if (M->MaterialDomain != MD_PostProcess)
				{
					UE_LOG(LogIVSmoke, Warning, TEXT("SmokeVisualMaterial must be PostProcess domain"));
					SmokeVisualMaterial = nullptr;
				}
				else
				{
					// Ensure shader compilation is complete before rendering
					SmokeVisualMaterial->EnsureIsComplete();
				}
			}
		}
	}
}
#endif
