#pragma once

#include "Components/ActorComponent.h"
#include "Persistence/GYPersistenceSubsystem.h"
#include "CharacterSaveComponent.generated.h"

struct FOnAttributeChangeData;

// [SERVER] 캐릭터별 저장 상태(version/dirty)와 저장 흐름 소유. PlayerState에 부착.
// 변경 → RequestSave()로 dirty 표시, in-flight 락으로 한 번에 하나만 전송 (전송 중 변경은 완료 후 한 번으로 뭉침).
// 저장은 캐릭터 통짜 blob — 부분 저장 없음. LoadAndApply 완료 전 저장은 게이팅 (version 미동기 상태 충돌 방지).
UCLASS()
class GY_API UCharacterSaveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterSaveComponent();

	// 모든 저장 트리거(델리게이트/체크포인트/주기)의 단일 진입점
	void RequestSave();

	// DB에서 로드 → IGYSaveable 컴포넌트들에 적용 + save_version 동기화
	void LoadAndApply();

	// 입장 시 1회 로드 보장. 이미 로드됐으면 스킵 — 재소환으로 init 체인이 다시 돌아도 세션 상태를 DB로 덮지 않는다
	void EnsureLoaded();

	bool IsLoaded() const { return bLoaded; }

	// 계정-캐릭터 1:1: 접속 옵션(?charId=, GYGameMode가 파싱) 또는 로컬 로그인 계정에서 지정.
	// 로드 시작 후에는 무시 — 세션 중 캐릭터 교체 없음
	void SetCharacterId(int64 InCharacterId);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TrySave();
	void OnSaveDone(const FGYSaveResult& Result);
	void OnLoadDone(const FGYLoadResult& Result);
	void OnLevelChanged(const FOnAttributeChangeData& Data);
	void OnPeriodicTimer();
	void ScheduleRetry();
	void OnRetryTimer();

	UGYPersistenceSubsystem* ResolvePersistence() const;

	// characters 테이블 level/xp 컬럼용 (data JSON 과 별도 denormalize)
	void ReadLevelAndXp(int32& OutLevel, int32& OutXp) const;

	// 미지정 시 dev stub(1) — gy.Persist 콘솔 흐름용. 저장 경로는 int32 유지 (이 규모에서 충분)
	int32 CharacterId = 1;
	bool bCharacterIdExplicit = false;

	int32 CachedSaveVersion = 0;
	bool bLoaded = false;
	bool bLoading = false;
	bool bSaving = false;
	bool bDirty = false;
	bool bApplying = false;

	FTimerHandle RetryTimerHandle;
	FTimerHandle PeriodicTimerHandle;
};
