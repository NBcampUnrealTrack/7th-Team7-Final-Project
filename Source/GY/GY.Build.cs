using UnrealBuildTool;

public class GY : ModuleRules
{
	public GY(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
			"NetCore",
			"DeveloperSettings",
			"DataBridge",
			"AIModule",
			"NavigationSystem",
			"GameplayMessageRuntime",
			"ModularGameplay",
			"GameFeatures",
			"CommonUI",
			"CommonInput",
			"GYUI",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
