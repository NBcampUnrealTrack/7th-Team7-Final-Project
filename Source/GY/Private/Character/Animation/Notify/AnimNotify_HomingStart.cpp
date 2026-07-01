#include "Character/Animation/Notify/AnimNotify_HomingStart.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "AIController.h"
#include "AbilitySystem/Abilities/Tasks/AbilityTask_HomeToTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Enemy/GYEnemyAIController.h"

AActor* UAnimNotify_HomingStart::ResolveHomingTarget(AActor* Owner)
{
	if (!Owner) return nullptr;

	// Player
	if (ULockOnComponent* LockOnComponent = Owner->FindComponentByClass<ULockOnComponent>())
	{
		if (AActor* Target = LockOnComponent->GetCurrentTarget())
		{
			return Target;
		}
	}

	// AI
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor)))
				{
					return Target;
				}
			}
		}
	}
	return nullptr;
}

void UAnimNotify_HomingStart::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase*, const FAnimNotifyEventReference&)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC) return;

	UGYGameplayAbility* GA = Cast<UGYGameplayAbility>(ASC->GetAnimatingAbility());
	if (!GA) return;

	AActor* Target = ResolveHomingTarget(Owner);
	if (!Target) return;

	UAbilityTask_HomeToTarget* Task = UAbilityTask_HomeToTarget::CreateHomeToTarget(
		GA, Target, Duration, MaxRotationSpeed, InterpSpeed, bUseConstantSpeed);
	if (Task) Task->ReadyForActivation();
}
