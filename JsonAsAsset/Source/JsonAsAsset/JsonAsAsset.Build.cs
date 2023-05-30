// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JsonAsAsset : ModuleRules
{
	public JsonAsAsset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"Json",
				"JsonUtilities",
				"UMG",
				"RenderCore",
				"HTTP",
				"DeveloperSettings",
				"Niagara"
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
				"InterchangePipelines",
				"MessageLog",
				"ToolWidgets",
				"PluginUtils",
				"RHI"
			}
		);

		if (Target.Version.MajorVersion == 4)
		{
			PrivateDependencyModuleNames.Add("AnimationDataController");
		}
	}
}