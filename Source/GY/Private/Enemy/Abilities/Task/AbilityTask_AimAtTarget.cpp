#include "Enemy/Abilities/Task/AbilityTask_AimAtTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

UAbilityTask_AimAtTarget* UAbilityTask_AimAtTarget::Create(
	UGameplayAbility* OwningAbility, FName InTargetBlackboardKey)
{
	UAbilityTask_AimAtTarget* Task = NewAbilityTask<UAbilityTask_AimAtTarget>(OwningAbility);
	Task->BlackboardKey = InTargetBlackboardKey;
	return Task;
}

void UAbilityTask_AimAtTarget::Activate()
{
	bTickingTask = true;
}

void UAbilityTask_AimAtTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
	if (!Char) return;

	AAIController* AIC = Cast<AAIController>(Char->GetController());
	if (!AIC) return;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey)) : nullptr;

	if (Target)
	{
		FVector EyeLoc; FRotator EyeRot;
		Char->GetActorEyesViewPoint(EyeLoc, EyeRot);
		const FRotator AimRot = (Target->GetActorLocation() - EyeLoc).Rotation();
		AIC->SetControlRotation(AimRot);
	}
	else
	{
		FRotator Fwd = Char->GetActorRotation();
		Fwd.Pitch = 0.f;
		AIC->SetControlRotation(Fwd);
	}
}

void UAbilityTask_AimAtTarget::OnDestroy(bool bInOwnerFinished)
{
	if (ACharacter* Char = Cast<ACharacter>(GetAvatarActor()))
	{
		if (AAIController* AIC = Cast<AAIController>(Char->GetController()))
		{
			FRotator Fwd = Char->GetActorRotation();
			Fwd.Pitch = 0.f;
			AIC->SetControlRotation(Fwd);
		}
	}
	Super::OnDestroy(bInOwnerFinished);
}
