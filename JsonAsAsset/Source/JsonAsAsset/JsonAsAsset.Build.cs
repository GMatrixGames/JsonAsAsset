// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JsonAsAsset : ModuleRules
{
	public JsonAsAsset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Json",
				"JsonUtilities",
				"MaterialEditor"

			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AnimationDataController",
				"MaterialEditor"
			}
		);

		if (Target.Version.MajorVersion == 4)
		{
			PrivateDependencyModuleNames.Add("AnimationDataController");
		}
	}
}