// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Drone : ModuleRules
{
	public Drone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		PublicIncludePaths.AddRange(new string[] {
			"Drone",
			"Drone/Variant_Platforming",
			"Drone/Variant_Platforming/Animation",
			"Drone/Variant_Combat",
			"Drone/Variant_Combat/AI",
			"Drone/Variant_Combat/Animation",
			"Drone/Variant_Combat/Gameplay",
			"Drone/Variant_Combat/Interfaces",
			"Drone/Variant_Combat/UI",
			"Drone/Variant_SideScrolling",
			"Drone/Variant_SideScrolling/AI",
			"Drone/Variant_SideScrolling/Gameplay",
			"Drone/Variant_SideScrolling/Interfaces",
			"Drone/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
