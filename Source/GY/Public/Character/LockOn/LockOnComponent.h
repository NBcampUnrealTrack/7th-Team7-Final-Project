#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "LockOnComponent.generated.h"


class AGYPlayerState;
class UAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	ULockOnComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void BindToASC(AGYPlayerState* PlayerState);

	AActor* GetCurrentTarget() const;
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

	void StartLockOn();
	void StopLockOn();

protected:
	UFUNCTION()
	void OnRep_CurrentTarget();

	void OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount);

	AActor* FindBestTarget() const;
	void UpdateRotationToTarget(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float MaxLockOnDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float RotationInterpSpeed = 8.f;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float TargetRetryInterval = 0.2f;

	FTimerHandle RetryTargetHandle;


private:
	void BroadcastLockOnMessage();
	void RetryFindTarget();

	UFUNCTION()
	void HandleTargetDied(AGYEnemyCharacterBase* DeadEnemy);

	void SwitchToBestTarget();

	void BindTargetDeathListener(AActor* Target);
	void UnbindTargetDeathListener(AActor* Target);


	FDelegateHandle InCombatTagHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	bool bSavedOrientToMovement = true;
	bool bSavedUseControllerRotationYaw = false;
};
