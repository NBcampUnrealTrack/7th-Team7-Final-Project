#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"


class AGYEnemyCharacterBase;
class AGYPlayerState;
class UAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	ULockOnComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void BindToASC(AGYPlayerState* PlayerState);

	AActor* GetCurrentTarget() const;
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

	void StartLockOn();
	void StopLockOn();

	void SetRotationSuppressed(bool bSuppressed) { bRotationSuppressed = bSuppressed; }

	UFUNCTION(Server, Reliable)
	void ServerSetLockOnTarget(AActor* NewTarget);
protected:
	UFUNCTION()
	void OnRep_CurrentTarget();

	void OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount);

	AActor* FindBestTarget() const;
	void UpdateRotationToTarget(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float MaxLockOnDistance = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float RotationInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	FGameplayTagContainer RotationBlockTags;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float TargetRetryInterval = 0.2f;

	FTimerHandle RetryTargetHandle;

	UPROPERTY(EditDefaultsOnly, Category="LockOn|Switch")
	float SwitchAccumulatorThreshold = 400.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn|Switch")
	float SwitchCooldown = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn|Switch")
	float SwitchAccumulatorDecayRate = 1.f;


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
	bool bRotationSuppressed = false;
	bool bOrientSuppressed = false;

	void ProcessTargetSwitchInput(float DeltaTime);
	AActor* FindDirectionalTarget(FVector2D Direction) const;

	float SquaredSwitchAccumulatorThreshold = 0.f;

	FVector2D SwitchAccumulator = FVector2D::ZeroVector;
	FVector2D PrevMousePosition = FVector2D::ZeroVector;
	bool bPrevMouseValid = false;
	float LastSwitchTime = 0.f;


};
