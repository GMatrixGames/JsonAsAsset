using System.IO;
using UnrealBuildTool;

public class Detex : ModuleRules
{
	public Detex(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty/detex"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty/detex"));

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject"
		});
	}
}