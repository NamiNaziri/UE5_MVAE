// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MVAE : ModuleRules
{
	public MVAE(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput","NNE",});
		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
