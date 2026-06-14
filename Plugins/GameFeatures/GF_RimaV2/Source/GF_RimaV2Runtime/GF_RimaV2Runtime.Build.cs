// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GF_RimaV2Runtime : ModuleRules
{
	public GF_RimaV2Runtime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


        PublicDependencyModuleNames.AddRange(
    new string[]
    {
        "Core",
        "CoreUObject",
        "Engine",
        "InputCore",
        "LyraGame",
        "ModularGameplay",
        "CommonGame",
        "GameplayAbilities",
        "GameplayTags",
        "GameplayTasks",
        "AIModule"
    }
);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
        "CoreUObject",
        "Engine",
        "Slate",
        "SlateCore",
        "LyraGame"
            }
        );


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
