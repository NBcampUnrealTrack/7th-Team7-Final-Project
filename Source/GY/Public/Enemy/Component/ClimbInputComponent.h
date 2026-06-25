#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "ClimbInputComponent.generated.h"

class UAbilitySystemComponent;
class ACharacter;
class UGYCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UClimbInputComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UClimbInputComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void BindToPawn(APawn* InPawn);
	void UnbindFromPawn();

protected:
	UFUNCTION()
	void OnClimbingTagChanged(const FGameplayTag Tag, int32 NewCount);


	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float ArrivedZTolerance = 30.f;

	TWeakObjectPtr<APawn> ControlledPawn;
	TWeakObjectPtr<UGYCharacterMovementComponent> CachedCMC;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FDelegateHandle ClimbingTagHandle;
};
