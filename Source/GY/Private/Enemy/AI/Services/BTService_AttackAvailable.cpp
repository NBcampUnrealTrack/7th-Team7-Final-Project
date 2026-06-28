#include "Enemy/AI/Services/BTService_AttackAvailable.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTService_AttackAvailable::UBTService_AttackAvailable()
{
	NodeName = TEXT("Attack Available");
	Interval = 0.5f;
	bCallTickOnSearchStart = true;
}

void UBTService_AttackAvailable::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
    if (!AIC) return;
	UE_LOG(LogTemp, Warning, TEXT("A_________1"));
    AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
    if (!Enemy) return;
	UE_LOG(LogTemp, Warning, TEXT("A_________2"));
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;
	UE_LOG(LogTemp, Warning, TEXT("A_________3"));
	CachedOwnerComp = OwnerComp.GetBlackboardComponent();

	UEnvQuery* EQSCheckDistance = (Cast<UEnvQuery>(BB->GetValueAsObject(EnemyBBKeys::EQSCheckDistance)));
	UEnvQuery* EQSCheckAngle = (Cast<UEnvQuery>(BB->GetValueAsObject(EnemyBBKeys::EQSCheckAngle)));

	if (!EQSCheckDistance || !EQSCheckAngle) return;
	UE_LOG(LogTemp, Warning, TEXT("A_________4"));
	UGYEnemyAttackAbilityBase* SelectedAbility = Cast<UGYEnemyAttackAbilityBase>(
			BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!SelectedAbility)
	{
		CachedOwnerComp->SetValueAsFloat(EnemyBBKeys::DistanceScore, 0.f);
		CachedOwnerComp->SetValueAsFloat(EnemyBBKeys::AngleScore, 0.f);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("A_________5"));
	FEnvQueryRequest DistanceQueryRequest(EQSCheckDistance, Enemy);
	DistanceQueryRequest.SetFloatParam(TEXT("AttackRange"), SelectedAbility->AttackRange * 1.2f);
	DistanceQueryRequest.SetFloatParam(TEXT("AttackRangeMin"), SelectedAbility->AttackRange * 1.2f);
	DistanceQueryRequest.Execute(EEnvQueryRunMode::SingleResult, this, &UBTService_AttackAvailable::OnDistanceEQSFinished);
	UE_LOG(LogTemp, Warning, TEXT("A_________6"));
	FEnvQueryRequest AngleQueryRequest(EQSCheckAngle, Enemy);
	AngleQueryRequest.SetFloatParam(TEXT("AttackRange"), SelectedAbility->AttackRange * 1.2f);
	AngleQueryRequest.SetFloatParam(TEXT("AttackRangeMin"), SelectedAbility->AttackRange * 1.2f);
	AngleQueryRequest.Execute(EEnvQueryRunMode::SingleResult, this, &UBTService_AttackAvailable::OnAngleEQSFinished);
	UE_LOG(LogTemp, Warning, TEXT("A_________7"));
}

void UBTService_AttackAvailable::OnDistanceEQSFinished(TSharedPtr<FEnvQueryResult> Result)
{
	UE_LOG(LogTemp, Warning, TEXT("A_________8"));
	float BestScore = Result->GetItemScore(0);
	CachedOwnerComp->SetValueAsFloat(EnemyBBKeys::DistanceScore, BestScore);
}
void UBTService_AttackAvailable::OnAngleEQSFinished(TSharedPtr<FEnvQueryResult> Result)
{
	UE_LOG(LogTemp, Warning, TEXT("A_________9"));
	float BestScore = Result->GetItemScore(0);
	CachedOwnerComp->SetValueAsFloat(EnemyBBKeys::AngleScore, BestScore);
}
