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
			"GY",
			"GameplayMessageRuntime",  // GameplayMessageSystem
			"CommonUI",                // Common UI
			"CommonInput",             // CommonUI 필수 의존
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
