// Copyright (c) 2026, Team SDB. All rights reserved.

using UnrealBuildTool;

public class IVSmoke : ModuleRules
{
	public IVSmoke(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseUnity = false;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Public API only - no Internal/Private engine headers required
		// Uses SubscribeToPostProcessingPass pattern with FPostProcessMaterialInputs

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RenderCore",
				"RHI",
				"Renderer",
				"Projects"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"NetCore"
			}
		);

		// Editor-only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
