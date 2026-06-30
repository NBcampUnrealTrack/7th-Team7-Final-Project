#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Components/ActorComponent.h"
#include "BossPhaseComponent.generated.h"

class UAbilitySystemComponent;
class AGYBossCharacterBase;
class UGYBossPhaseAbility;

/**
 * 보스 페이즈 전환 트리거 정의.
 *
 * 보스 체력이 HealthRatio 이하로 떨어지는 순간 1회 발동되어,
 * PhaseAbilityClass 가 PhaseComponent 큐에 들어가고 StateTree 의 PhaseAction 이 이를 활성화한다.
 */
USTRUCT(BlueprintType)
struct FBossPhaseTrigger
{
	GENERATED_BODY()

	/** 이 비율 이하로 보스 체력이 떨어지면 트리거 발동 (0.0 ~ 1.0) */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthRatio = 0.5f;

	/** 트리거 발동 시 활성화할 페이즈 어빌리티 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGYBossPhaseAbility> PhaseAbilityClass;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossPhaseQueued, const FBossPhaseTrigger&, Trigger);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossPhaseStarted, TSubclassOf<UGameplayAbility>, AbilityClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossPhaseFinished, TSubclassOf<UGameplayAbility>, AbilityClass);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UBossPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossPhaseComponent();

	/** 보스룸 트리거 발동해서 전투 시작 시점 */
	void InitializeForEncounter(const TArray<FBossPhaseTrigger>& InTriggers);

	UFUNCTION(BlueprintPure, Category = "Boss|Phase")
	bool HasPendingPhaseAction() const { return PendingQueue.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "Boss|Phase")
	TSubclassOf<UGameplayAbility> PeekNextPhaseAbility() const;

	TSubclassOf<UGameplayAbility> PopNextPhaseAbility();

	void EnqueuePhaseAbilities(const TArray<TSubclassOf<UGameplayAbility>>& Abilities);

	void NotifyPhaseStarted(TSubclassOf<UGameplayAbility> AbilityClass);

	void NotifyPhaseFinished(TSubclassOf<UGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintPure, Category = "Boss|Phase")
	int32 GetPendingCount() const { return PendingQueue.Num(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Phase")
	int32 GetTriggeredPhaseCount() const;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase")
	FOnBossPhaseQueued OnPhaseQueued;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase")
	FOnBossPhaseStarted OnPhaseStarted;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase")
	FOnBossPhaseFinished OnPhaseFinished;

	void ServerSetAOEWindow(bool bActive, float Duration);

	UFUNCTION(BlueprintPure, Category = "Boss|AOE")
	bool IsAOEWindowActive() const { return bAOEWindowActive; }

	UFUNCTION(BlueprintPure, Category = "Boss|AOE")
	float GetAOEWindowDuration() const { return AOEWindowDuration; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual  void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void BindToHealthAttribute();
	void UnbindFromHealthAttribute();

	void OnHealthChanged(const FOnAttributeChangeData& Data);

	void EvaluatePhaseTriggers();

	UFUNCTION()
	void OnRep_TriggeredFlags();

	UFUNCTION()
	void OnRep_AOEWindowActive();

	void BroadcastAOEWindowMessage() const;

protected:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Boss|Phase")
	TArray<FBossPhaseTrigger> PhaseTriggers;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UGameplayAbility>> PendingQueue;

	UPROPERTY(ReplicatedUsing = OnRep_TriggeredFlags, Transient)
	TArray<bool> TriggeredFlags;

	UPROPERTY(ReplicatedUsing = OnRep_AOEWindowActive, Transient)
	bool bAOEWindowActive = false;

	UPROPERTY(Replicated, Transient)
	float AOEWindowDuration = 0.f;

	float LastObservedRatio = 1.f;

	bool bInitialized = false;

	FDelegateHandle HealthChangeHandle;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
