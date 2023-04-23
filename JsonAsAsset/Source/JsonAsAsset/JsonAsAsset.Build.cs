// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JsonAsAsset : ModuleRules
{
	public JsonAsAsset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDefinitions.Add((Target.Version.BranchName ?? "UE").StartsWith("++Pinnacle") ? "IS_PINNACLE=1" : "IS_PINNACLE=0");

		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"Json",
				"JsonUtilities",
				"UMG",
				"RenderCore",
				"HTTP",
				"DeveloperSettings"
			}
		);

		PrivateDependencyModuleNames.AddRange(new string[]
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
				"MaterialEditor",
				"ImageWriteQueue",
				"Landscape",
				"AssetTools",
				"EditorStyle",
				"Settings",
				"PhysicsCore",
				"InterchangeEngine",
				"InterchangeNodes",
				"InterchangeCommonParser",
				"InterchangeFactoryNodes",
				"InterchangeImport",
				"InterchangePipelines"
			}
		);

		if (Target.Version.MajorVersion == 4)
		{
			PrivateDependencyModuleNames.Add("AnimationDataController");
		}
	}
}