#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "GYExperienceManager.generated.h"

// PIE 멀티 인스턴스에서 게임 피쳐 플러그인 활성/비활성을 ref-count로 조율한다.
// 한 PIE 창이 EndPlay로 피쳐를 끄면 다른 창의 피쳐까지 죽는 것을 막는다.
// 엔진 레벨 서브시스템이어야 PIE 창(=별도 GameInstance)들을 가로질러 카운트가 공유된다.
UCLASS()
class GY_API UGYExperienceManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	// 피쳐 활성화 시 호출. 에디터에서만 ref-count 증가.
	static void NotifyOfPluginActivation(const FString PluginURL);

	// 피쳐 비활성화 요청. 마지막 참조일 때만 true 반환(실제 비활성 허용). 에디터 밖에선 항상 true.
	static bool RequestToDeactivatePlugin(const FString PluginURL);

private:
#if WITH_EDITOR
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
#endif
};
