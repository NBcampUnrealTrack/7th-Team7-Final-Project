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
			"GY"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
