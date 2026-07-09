#include "Enemy/AI/Tasks/BTTask_ExecuteSelectedAbility.h"

#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"

UBTTask_ExecuteSelectedAbility::UBTTask_ExecuteSelectedAbility()
{
	NodeName = TEXT("Execute Selected Ability");
	bNotifyTaskFinished = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ExecuteSelectedAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return EBTNodeResult::Failed;

	UGYEnemyAttackAbilityBase* SelectedAbility = Cast<UGYEnemyAttackAbilityBase>(
		BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!SelectedAbility) return EBTNodeResult::Failed;


	FGameplayAbilitySpecHandle Handle;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability == SelectedAbility)
		{
			Handle = Spec.Handle;
			break;
		}
	}

	if (!Handle.IsValid())
	{
		// ASC에서 제거된 어빌리티(페이즈 교체 등)가 BB에 남은 경우 — 비워서 재선택 유도
		BB->SetValueAsObject(EnemyBBKeys::SelectedAbility, nullptr);
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;
	CachedASC = ASC;
	CachedAbilityHandle = Handle;

	ASC->OnAbilityEnded.AddUObject(this, &UBTTask_ExecuteSelectedAbility::OnASCAbilityEnded);

	if (!ASC->TryActivateAbility(Handle))
	{
		ASC->OnAbilityEnded.RemoveAll(this);
		CachedOwnerComp = nullptr;
		CachedASC = nullptr;
		BB->SetValueAsObject(EnemyBBKeys::SelectedAbility, nullptr);
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsObject(EnemyBBKeys::LastUsedAbility, SelectedAbility);
	return EBTNodeResult::InProgress;
}

void UBTTask_ExecuteSelectedAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	if (CachedASC.IsValid())
	{
		CachedASC->OnAbilityEnded.RemoveAll(this);
		if (TaskResult == EBTNodeResult::Aborted)
			CachedASC->CancelAllAbilities();
		CachedASC = nullptr;
	}
	if (CachedOwnerComp.IsValid())
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsObject(EnemyBBKeys::SelectedAbility, nullptr);
		}
		CachedOwnerComp = nullptr;
	}
}

void UBTTask_ExecuteSelectedAbility::OnASCAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilitySpecHandle != CachedAbilityHandle) return;
	if (CachedASC.IsValid())
	{
		CachedASC->OnAbilityEnded.RemoveAll(this);
	}
	if (CachedOwnerComp.IsValid())
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	}
}
