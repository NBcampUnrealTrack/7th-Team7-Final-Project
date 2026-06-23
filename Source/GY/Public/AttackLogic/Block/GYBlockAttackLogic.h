#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AttackLogic/Block/GYBlockAttackFragment.h"
#include "GYBlockAttackLogic.generated.h"

// 막기 추가입력 로직 — 막기 피격 후 일정 시간 내 공격 입력 시 추가 공격 실행
UCLASS()
class GY_API UGYBlockAttackLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	UGYBlockAttackLogic();

	// 추가입력 트리거로 사용할 게임플레이 이벤트 태그 (기본값: Event_Input_Attack)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlockAttack")
	FGameplayTag TriggerEventTag;

	// GYANS_AttackTrace가 트레이스 형태 결정을 위해 호출
	const FGYCollisionShapeData* GetCurrentCollisionData() const;

	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	// 막기 피격 후 추가입력 윈도우 열기 및 타임아웃 타이머 시작
	void OpenWindow();
	// 윈도우 닫기 및 타이머 해제
	void CloseWindow();
	// 추가입력 확인 후 세트 몽타주 재생, 히트 임팩트 설정
	void ExecuteBlockAttack();
	// DoTrace 이벤트 수신 시 대상에 데미지 적용
	void ApplyHit(const FGameplayEventData& Payload);

	UFUNCTION()
	// 추가 공격 몽타주 종료 후 블록 홀드 자세로 복귀
	void OnAttackMontageFinished();

	UFUNCTION()
	// 윈도우 타임아웃 완료 시 호출
	void OnWindowTimedOut();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	// 현재 재생 중인 세트 포인터 — 몽타주 종료 시 null 초기화
	const FGYBlockAttackSet* CachedAttackSet = nullptr;
	bool bWindowOpen = false;
	float CachedWindowTimeout = 2.0f;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> WindowTimeoutTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> AttackMontageTask;
};
