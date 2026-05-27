#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GameplayTagContainer.h"
#include "GYAttackInputLogic.generated.h"

class UAnimMontage;
struct FGYCollisionShapeData;
struct FGYChargeMontageSet;
struct FGYChargeData;

// 콤보 + 차지를 단일 어빌리티에서 처리.
// 입력은 Phase 1 라우팅으로 활성화되고, 활성 중 입력은 Ability::InputPressed/Released 경유로 받음.
// 탭/연타 → 콤보, 임계시간 이상 홀드 → 차지. (구 Event.Input.* + PlayerState 타이머 대체)
UCLASS()
class GY_API UGYAttackInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;
	virtual void OnInputPressed() override;
	virtual void OnInputReleased() override;

	const FGYCollisionShapeData* GetCurrentCollisionData() const;

	// 이 시간 이상 홀드하면 차지 진입
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float HoldToChargeTime = 0.3f;

private:
	void StartCombo();
	void PlayComboMontage();
	void AdvanceCombo();
	void EnterCharge();
	void ExecuteChargeAttack();

	void OnChargeThresholdReached();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;

	// 콤보 상태
	int32 ComboIndex = 0;
	int32 MaxComboCount = 0;
	int32 ComboIndexAtWindowOpen = 0;
	bool bWindowOpen = false;
	bool bPendingCombo = false;
	bool bReady = false;
	// 누르고 있는 동안 OnInputPressed가 프레임마다 들어오므로 눌림 엣지만 처리하기 위한 가드
	bool bInputHeld = false;
	// 첫타는 탭/홀드 판정 후에 시작되므로, 콤보가 실제로 시작됐는지 추적
	bool bComboStarted = false;
	const TArray<TObjectPtr<UAnimMontage>>* CachedComboMontages = nullptr;
	const TArray<FGYCollisionShapeData>* CachedComboCollisions = nullptr;

	// 차지 상태
	bool bCharging = false;
	float ChargeStartTime = 0.f;
	const FGYChargeMontageSet* CachedChargeMontageSet = nullptr;
	const FGYChargeData* CachedChargeData = nullptr;
	const TArray<FGYCollisionShapeData>* CachedChargeCollisions = nullptr;

	FTimerHandle ChargeThresholdTimer;
	FTimerHandle MaxChargeTimer;
	FTimerHandle MontageEndTimer;
};
