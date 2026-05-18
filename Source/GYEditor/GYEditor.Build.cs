using UnrealBuildTool;

public class GYEditor : ModuleRules
{
    public GYEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "WorldPartitionEditor",
                "Blutility",
                "UMGEditor",
                "UnrealEd",
                "EditorScriptingUtilities",
                "GY",

            }
        );

    }
}
