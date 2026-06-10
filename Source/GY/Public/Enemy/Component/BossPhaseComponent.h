#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Components/ActorComponent.h"
#include "BossPhaseComponent.generated.h"

class UAbilitySystemComponent;
class AGYBossCharacterBase;

USTRUCT(BlueprintType)
struct FBossPhaseTrigger
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly)
	FName DebugName = NAME_None;
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
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual  void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void BindToHealthAttribute();
	void UnbindFromHealthAttribute();

	void OnHealthChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void OnRep_TriggeredFlags();

protected:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Boss|Phase")
	TArray<FBossPhaseTrigger> PhaseTriggers;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UGameplayAbility>> PendingQueue;

	UPROPERTY(ReplicatedUsing = OnRep_TriggeredFlags, Transient)
	TArray<bool> TriggeredFlags;

	float LastObservedRatio = 1.f;

	bool bInitialized = false;

	FDelegateHandle HealthChangeHandle;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
