#include "Enemy/AI/StateTree/StaggerReactTask.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Enemy/GYEnemyCharacterBase.h"

EStateTreeRunStatus FStaggerReactTask::EnterState(FStateTreeExecutionContext& Context,
                                                  const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;
	Data.bMontagePlaying = false;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI) return EStateTreeRunStatus::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AI->GetPawn());
	if (!Enemy) return EStateTreeRunStatus::Failed;

	if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
	{
		if (!Data.CancelAbilitiesTags.IsEmpty())
		{
			FGameplayTagContainer NoBlock;
			ASC->CancelAbilities(&Data.CancelAbilitiesTags, &NoBlock);
		}
	}

	AI->StopMovement();

	FGameplayTag MontageTag;
	for (const FStateTreeSharedEvent& Event : Context.GetEventsToProcessView())
	{
		if (Event.IsValid())
		{
			MontageTag = Event->Tag;
			break;
		}
	}
	if (!MontageTag.IsValid())
	{
		MontageTag = Data.FallbackMontageTag;
	}

	if (MontageTag.IsValid())
	{
		if (UAnimMontage* Montage = Enemy->GetMontageByTag(MontageTag))
		{
			if (USkeletalMeshComponent* Mesh = Enemy->GetMesh())
			{
				if (UAnimInstance* Anim = Mesh->GetAnimInstance())
				{
					const float Duration = Anim->Montage_Play(Montage);
					Data.bMontagePlaying = (Duration > 0.f);
				}
			}
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStaggerReactTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	AGYEnemyCharacterBase* Enemy = AI ? Cast<AGYEnemyCharacterBase>(AI->GetPawn()) : nullptr;

	if (Data.bMontagePlaying && Enemy)
	{
		if (USkeletalMeshComponent* Mesh = Enemy->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				if (!Anim->IsAnyMontagePlaying())
				{
					return EStateTreeRunStatus::Succeeded;
				}
			}
		}
	}

	if (Data.ElapsedTime > Data.MaxDuration) return EStateTreeRunStatus::Succeeded;

	return EStateTreeRunStatus::Running;
}
