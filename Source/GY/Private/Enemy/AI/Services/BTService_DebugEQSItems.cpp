#include "Enemy/AI/Services/BTService_DebugEQSItems.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTService_DebugEQSItems::UBTService_DebugEQSItems()
{
	NodeName = TEXT("Debug EQS All Items");
	Interval = 0.5f;
}

void UBTService_DebugEQSItems::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (!QueryTemplate) return;


	CachedWorld = OwnerComp.GetWorld();

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	UGYEnemyAttackAbilityBase* SelectedAbility = Cast<UGYEnemyAttackAbilityBase>(
			BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!SelectedAbility) return ;
	FEnvQueryRequest QueryRequest(QueryTemplate, AIC->GetPawn());
	QueryRequest.SetFloatParam(TEXT("AttackRange"), SelectedAbility->AttackRange * 0.95f);
	QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), SelectedAbility->AttackRange * 0.5f);
	QueryRequest.Execute(EEnvQueryRunMode::AllMatching,
		this, &UBTService_DebugEQSItems::OnQueryFinished);
}

void UBTService_DebugEQSItems::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (!Result.IsValid() || !CachedWorld.IsValid()) return;

	float MaxScore = 0.f;
	for (auto& Item : Result->Items)
	{
		MaxScore = FMath::Max(MaxScore, Item.Score);
	}

	for (int32 i = 0; i < Result->Items.Num(); i++)
	{
		FVector Pos = Result->GetItemAsLocation(i);
		float Score = Result->Items[i].Score;
		float NormalizedScore = (MaxScore > 0.f) ? (Score / MaxScore) : 0.f;

		FColor Color = FLinearColor::LerpUsingHSV(
			FLinearColor::Blue, FLinearColor::Red, NormalizedScore).ToFColor(true);

		DrawDebugSphere(CachedWorld.Get(), Pos, 30.f, 8, Color, false, 0.6f, 0, 1.5f);
		DrawDebugString(CachedWorld.Get(), Pos + FVector(0, 0, 50.f),
			FString::Printf(TEXT("%.2f"), Score),
			nullptr, Color, 0.6f);
	}
}
