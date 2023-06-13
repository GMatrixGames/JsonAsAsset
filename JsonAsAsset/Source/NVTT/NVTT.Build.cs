using System.IO;
using UnrealBuildTool;

public class NVTT : ModuleRules
{
    public NVTT(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty/nvtt"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty/nvtt"));

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "ApplicationCore"
        });
    }
}