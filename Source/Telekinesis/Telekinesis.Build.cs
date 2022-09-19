// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Telekinesis : ModuleRules
{
	public Telekinesis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "Niagara" });

	}
}
