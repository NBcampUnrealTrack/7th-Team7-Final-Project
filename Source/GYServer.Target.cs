using UnrealBuildTool;
using System.Collections.Generic;

public class GYServerTarget : TargetRules
{
	public GYServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// Shipping 서버도 GY.log 유지 — EC2 운영 장애 추적은 로그가 유일한 단서.
		// 엔진 기본값 수정이라 공유 빌드 환경 검사에 걸림 — installed build 엔진은 Unique 환경이
		// 불가하므로 override 로 강제 (로깅 매크로 차이만 있어 실질 무해)
		bOverrideBuildEnvironment = true;
		bUseLoggingInShipping = true;

		ExtraModuleNames.AddRange( new string[] { "GY" } );
	}

}
