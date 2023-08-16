// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JsonAsAsset : ModuleRules
{
	public JsonAsAsset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"Json",
			"JsonUtilities",
			"UMG",
			"RenderCore",
			"HTTP"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"InputCore",
			"UnrealEd",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"MaterialEditor",
			"Landscape",
			"AssetTools",
			"EditorStyle",
			"Settings",
			"MessageLog",
			"RHI",
			"Detex",
			"NVTT",
			"MainFrame"
		});
	}
}
