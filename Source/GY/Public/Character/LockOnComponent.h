#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"


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
	void BindToASC(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintPure, Category="LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="LockOn")
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

protected:
	UFUNCTION()
	void OnRep_CurrentTarget();

	void OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount);
	void StartLockOn();
	void StopLockOn();
	AActor* FindBestTarget() const;
	void UpdateRotationToTarget(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float MaxLockOnDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	float RotationInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = ECC_Pawn;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	TWeakObjectPtr<AActor> CurrentTarget;

private:
	FDelegateHandle InCombatTagHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
