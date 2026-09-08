using UnrealBuildTool;

public class Proxima : ModuleRules
{
    public Proxima(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Keep unity disabled while the foundation is small so missing includes
        // are caught instead of being hidden by another translation unit.
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore"
        });
    }
}
