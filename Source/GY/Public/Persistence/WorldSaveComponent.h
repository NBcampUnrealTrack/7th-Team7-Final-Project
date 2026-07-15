#pragma once

#include "Components/ActorComponent.h"
#include "Persistence/GYPersistenceSubsystem.h"
#include "WorldSaveComponent.generated.h"

// [SERVER] 월드 상태(퀘스트/월드레벨/시간/리셋)의 저장 흐름 소유. GameState 에 부착.
// 캐릭터(CharacterSaveComponent)와 같은 dirty/in-flight/CAS 모델. 차이점:
//  - 로드 3분기: Success=복원 / NotFound=행 생성 후 신규 월드 / Failure=저장 봉쇄+재시도.
//    Failure 를 신규로 오판하면 주기 저장이 월드를 빈 값으로 덮으므로 반드시 구분한다.
//  - 로드 완료 전 플레이어 입장 게이팅 (GameMode 가 IsWorldStateReady 를 PlayerCanRestart 에 합류)
//  - 월드파티션 맵에서만 동작 — 메뉴 맵 등 비 WP 월드는 비활성
UCLASS()
class GY_API UWorldSaveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWorldSaveComponent();

	// 모든 저장 트리거(퀘스트 변경/주기/콘솔)의 단일 진입점
	void RequestSave();

	// DB에서 로드 → 섹션 복원 + save_version 동기화
	void LoadAndApply();

	// 입장 게이팅 조건: 월드 상태가 복원(또는 신규 확정)되기 전까지 false
	bool IsWorldStateReady() const { return bLoaded; }

	// 강제 종료 경로(콘솔 닫기 등)에서 티어다운 전에 마지막 스냅샷 발사.
	// PersistenceSubsystem 이 OnEnginePreExit 에서 호출 — EndPlay 는 이후 중복 전송을 스킵한다
	void FlushForShutdown();

	// 이 월드에서 영속이 성립하는지 (WP 맵 + 서버 권위). 게이팅 판단에서 비활성 월드는 항상 통과
	bool IsPersistenceEnabled() const { return bEnabled; }

	int64 GetWorldId() const { return WorldId; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TrySave();
	void OnLoadDone(const FGYLoadResult& Result);
	void OnSaveDone(const FGYSaveResult& Result);
	void OnCreateDone(const FGYSaveResult& Result);
	void OnPeriodicTimer();
	void ScheduleRetry();
	void OnRetryTimer();
	void NotifyWorldStateReady();

	UGYPersistenceSubsystem* ResolvePersistence() const;

	// 섹션 컨테이너 JSON 직렬화/복원 — quest / world / reset
	FString BuildSaveData() const;
	void ApplyLoadedData(const TSharedPtr<FJsonObject>& Data);

	// worlds.world_level denorm 컬럼용 (권위 값은 data.world.level)
	int32 ReadWorldLevelDenorm() const;

	// 서버 실행 인자 -WorldId=N (기본 1 — 팀 단일 월드)
	int64 WorldId = 1;

	bool bEnabled = false;
	int32 CachedSaveVersion = 0;
	bool bLoaded = false;
	bool bLoading = false;
	bool bSaving = false;
	bool bDirty = false;
	bool bShutdownFlushed = false;

	FTimerHandle RetryTimerHandle;
	FTimerHandle PeriodicTimerHandle;
};
