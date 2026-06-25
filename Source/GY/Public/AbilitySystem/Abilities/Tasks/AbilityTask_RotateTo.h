#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_RotateTo.generated.h"

class AGYCharacter;

UCLASS()
class GY_API UAbilityTask_RotateTo : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAbilityTask_RotateTo* Create(
		UGameplayAbility* OwningAbility,
		AGYCharacter* Character,
		float StartYaw,
		float TargetYaw,
		float LerpTime);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	TWeakObjectPtr<AGYCharacter> TargetCharacter;
	float StartYaw = 0.f;
	float DeltaYaw = 0.f;
	float LerpDuration = 0.f;
	float Elapsed = 0.f;
};
