#include "Enemy/AI/Tasks/BTTask_UpdateAttackPos.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTTask_UpdateAttackPos::UBTTask_UpdateAttackPos()
{
	NodeName = TEXT("Update Attack Position");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_UpdateAttackPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	UGYEnemyAttackAbilityBase* SelectedAbility = Cast<UGYEnemyAttackAbilityBase>(
		BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!SelectedAbility) return EBTNodeResult::Failed;

	UEnvQuery* SelectedEQS = (SelectedAbility->AttackType == EGYEnemyAttackType::Ranged)
	? RangedEQSAsset
	: MeleeEQSAsset;

	if (!SelectedEQS) return EBTNodeResult::Failed;

	CachedOwnerComp = &OwnerComp;

	FEnvQueryRequest QueryRequest(SelectedEQS, Enemy);
	if (SelectedAbility->AttackType == EGYEnemyAttackType::Melee)
	{
		QueryRequest.SetFloatParam(TEXT("AttackRange"), SelectedAbility->AttackRange * 0.7f);
		QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), SelectedAbility->AttackRange * 0.3f);
	}
	else
	{
		QueryRequest.SetFloatParam(TEXT("AttackRange"), SelectedAbility->AttackRange * 0.95f);
		QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), SelectedAbility->AttackRange * 0.5f);
	}
	QueryRequest.Execute(EEnvQueryRunMode::SingleResult, this, &UBTTask_UpdateAttackPos::OnEQSFinished);

	return EBTNodeResult::InProgress;
}
void UBTTask_UpdateAttackPos::OnEQSFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (!Result.IsValid() || Result->IsAborted() || !CachedOwnerComp)
	{
		if (CachedOwnerComp)
			FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Result->Items.Num() == 0)
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Failed);
		CachedOwnerComp = nullptr;
		return;
	}

	FVector BestLocation = Result->GetItemAsLocation(0);
	CachedOwnerComp->GetBlackboardComponent()->SetValueAsVector(EnemyBBKeys::AttackPosition, BestLocation);

	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	CachedOwnerComp = nullptr;
}
