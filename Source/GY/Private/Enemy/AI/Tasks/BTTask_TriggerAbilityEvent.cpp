#include "Enemy/AI/Tasks/BTTask_TriggerAbilityEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"

UBTTask_TriggerAbilityEvent::UBTTask_TriggerAbilityEvent()
{
	NodeName = TEXT("Trigger Ability Event");
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_TriggerAbilityEvent::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	if (!EventTag.IsValid()) return EBTNodeResult::Failed;

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	ActiveAbility = nullptr;
	FDelegateHandle ActivatedHandle = ASC->AbilityActivatedCallbacks.AddUObject(
		this, &UBTTask_TriggerAbilityEvent::OnAbilityActivated);

	FGameplayEventData Payload;
	Payload.Instigator = Enemy;
	Payload.Target = Enemy;
	Payload.EventTag = EventTag;

	const int32 Triggered = ASC->HandleGameplayEvent(EventTag, &Payload);

	ASC->AbilityActivatedCallbacks.Remove(ActivatedHandle);

	if (Triggered == 0 || !ActiveAbility) return EBTNodeResult::Failed;

	CachedOwnerComp = &OwnerComp;
	ActiveAbility->OnGameplayAbilityEnded.AddUObject(
		this, &UBTTask_TriggerAbilityEvent::OnAbilityEnded);

	return EBTNodeResult::InProgress;
}

void UBTTask_TriggerAbilityEvent::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	if (ActiveAbility)
	{
		ActiveAbility->OnGameplayAbilityEnded.RemoveAll(this);

		if (TaskResult == EBTNodeResult::Aborted)
		{
			if (UAbilitySystemComponent* ASC =
				ActiveAbility->GetAbilitySystemComponentFromActorInfo())
			{
				ASC->CancelAbility(ActiveAbility);
			}
		}
		ActiveAbility = nullptr;
	}
	CachedOwnerComp = nullptr;
}

void UBTTask_TriggerAbilityEvent::OnAbilityEnded(UGameplayAbility* Ability)
{
	if (!CachedOwnerComp) return;
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}

void UBTTask_TriggerAbilityEvent::OnAbilityActivated(UGameplayAbility* Ability)
{
	if (ActiveAbility) return;
	ActiveAbility = Ability;
}
