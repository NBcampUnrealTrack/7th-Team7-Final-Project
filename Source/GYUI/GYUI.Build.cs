using UnrealBuildTool;

public class GYUI : ModuleRules
{
	public GYUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"UMG",
			"GameplayMessageRuntime",
			"CommonUI",
			"CommonInput",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"DeveloperSettings",
			"GY",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
