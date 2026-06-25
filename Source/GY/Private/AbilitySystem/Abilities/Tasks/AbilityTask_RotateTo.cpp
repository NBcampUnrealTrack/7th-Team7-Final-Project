#include "AbilitySystem/Abilities/Tasks/AbilityTask_RotateTo.h"
#include "Character/GYCharacter.h"

UAbilityTask_RotateTo* UAbilityTask_RotateTo::Create(
	UGameplayAbility* OwningAbility,
	AGYCharacter* Character,
	float InStartYaw,
	float TargetYaw,
	float LerpTime)
{
	UAbilityTask_RotateTo* Task = NewAbilityTask<UAbilityTask_RotateTo>(OwningAbility);
	Task->TargetCharacter = Character;
	Task->StartYaw = InStartYaw;
	Task->DeltaYaw = FRotator::NormalizeAxis(TargetYaw - InStartYaw);
	Task->LerpDuration = LerpTime;
	Task->Elapsed = 0.f;
	Task->bTickingTask = (LerpTime > 0.f && !FMath::IsNearlyZero(Task->DeltaYaw));
	return Task;
}

void UAbilityTask_RotateTo::Activate()
{
	if (!bTickingTask)
	{
		if (!FMath::IsNearlyZero(DeltaYaw))
		{
			if (AGYCharacter* Character = TargetCharacter.Get())
			{
				FRotator NewRot = Character->GetActorRotation();
				NewRot.Yaw = StartYaw + DeltaYaw;
				Character->SetActorRotation(NewRot);
			}
		}
		EndTask();
	}
}

void UAbilityTask_RotateTo::TickTask(float DeltaTime)
{
	AGYCharacter* Character = TargetCharacter.Get();
	if (!Character)
	{
		EndTask();
		return;
	}

	Elapsed += DeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / LerpDuration, 0.f, 1.f);
	const float EasedAlpha = 1.f - FMath::Square(1.f - Alpha);

	FRotator NewRot = Character->GetActorRotation();
	NewRot.Yaw = StartYaw + DeltaYaw * EasedAlpha;
	Character->SetActorRotation(NewRot);

	if (Alpha >= 1.f)
	{
		EndTask();
	}
}
