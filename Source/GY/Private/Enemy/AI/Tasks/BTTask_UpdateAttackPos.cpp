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

	if (!EQSAsset) return EBTNodeResult::Failed;

	CachedOwnerComp = &OwnerComp;

	FEnvQueryRequest QueryRequest(EQSAsset, Enemy);
	QueryRequest.SetFloatParam(TEXT("AttackRange"), SelectedAbility->AttackRange * 0.5f);
	QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), SelectedAbility->AttackRange * 0.3f);
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

	FVector BestLocation = Result->GetItemAsLocation(0);
	CachedOwnerComp->GetBlackboardComponent()->SetValueAsVector(EnemyBBKeys::AttackPosition, BestLocation);

	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	CachedOwnerComp = nullptr;
}
