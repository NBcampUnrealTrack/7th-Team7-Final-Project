#include "Enemy/AI/StateTree/CCReactTask.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "Enemy/GYEnemyCharacterBase.h"

EStateTreeRunStatus FCCReactTask::EnterState(FStateTreeExecutionContext& Context,
                                             const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;

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

		if (Data.WaitTag.IsValid() && !ASC->HasMatchingGameplayTag(Data.WaitTag))
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	AI->StopMovement();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCCReactTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	AGYEnemyCharacterBase* Enemy = AI ? Cast<AGYEnemyCharacterBase>(AI->GetPawn()) : nullptr;

	if (Enemy)
	{
		if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
		{
			if (Data.WaitTag.IsValid() && !ASC->HasMatchingGameplayTag(Data.WaitTag))
			{
				return EStateTreeRunStatus::Succeeded;
			}
		}
	}

	if (Data.ElapsedTime > Data.SafetyTimeout)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}
