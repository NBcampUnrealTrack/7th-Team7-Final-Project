using UnrealBuildTool;
using System.Collections.Generic;

public class GYServerTarget : TargetRules
{
	public GYServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "GY" } );
	}
}
