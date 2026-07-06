// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/Notify/AnimNotify_DashStart.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "AbilitySystem/Abilities/Tasks/AbilityTask_DashToTarget.h"
#include "AbilitySystem/Abilities/Tasks/AbilityTask_HomeToTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Enemy/GYEnemyAIController.h"

void UAnimNotify_DashStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                   const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC) return;

	UGYGameplayAbility* GA = Cast<UGYGameplayAbility>(ASC->GetAnimatingAbility());
	if (!GA) return;

	AActor* Target = ResolveHomingTarget(Owner);
	if (!Target) return;

	UAbilityTask_DashToTarget* Task = UAbilityTask_DashToTarget::CreateDashToTarget(GA,Target, DashSpeed, StopDistance);
	if (Task) Task->ReadyForActivation();
}

AActor* UAnimNotify_DashStart::ResolveHomingTarget(AActor* Owner)
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
