#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbInputComponent.generated.h"

class ACharacter;
class UGYCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UClimbInputComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UClimbInputComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float ArrivedZTolerance = 30.f;

	TWeakObjectPtr<ACharacter> OwnerCharacter;
	TWeakObjectPtr<UGYCharacterMovementComponent> CachedCMC;
};
