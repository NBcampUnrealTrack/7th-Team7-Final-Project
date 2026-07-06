using UnrealBuildTool;

public class GYEditor : ModuleRules
{
    public GYEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "GY",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "GameplayAbilities",
                "GameplayTags",
                "WorldPartitionEditor",
                "Blutility",
                "UMGEditor",
                "UnrealEd",
                "AssetRegistry",
                "EditorScriptingUtilities",
                "GY",
				"EditorFramework",
				"GraphEditor",
				"BlueprintGraph",
				"ToolMenus",
				"PropertyEditor",
				"DetailCustomizations",
				"AnimationBlueprintLibrary",


            }
        );

    }
}
