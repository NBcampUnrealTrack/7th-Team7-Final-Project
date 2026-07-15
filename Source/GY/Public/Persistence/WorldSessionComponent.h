#pragma once

#include "Components/ActorComponent.h"
#include "WorldSessionComponent.generated.h"

// [SERVER] 월드 목록(worlds 테이블)에 이 세션을 등록·갱신한다 (하트비트). GameState 에 부착.
// -PublicAddr=ip:port 실행 인자가 있을 때만 동작 — 오케스트레이터가 스폰한 서버만 목록에 등록되고,
// 로컬 개발 서버(-PublicAddr 없음)는 목록을 오염시키지 않는다.
// 갱신 실패/프로세스 급사는 오케스트레이터가 stale 하트비트 리핑으로 수습 (여기선 fire-and-forget)
UCLASS()
class GY_API UWorldSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWorldSessionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnHeartbeatTimer();

	int64 WorldId = 1;
	FString PublicAddr;
	bool bEnabled = false;

	FTimerHandle HeartbeatTimerHandle;
};
