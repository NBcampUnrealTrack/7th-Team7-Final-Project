#include "Enemy/AI/Services/BTService_SelectAbility.h"

#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "Enemy/AI/Tasks/BTTask_UpdateAttackPos.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTService_SelectAbility::UBTService_SelectAbility()
{
	NodeName = TEXT("Select Ability");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
	bCallTickOnSearchStart = true;
}

void UBTService_SelectAbility::OnEQSFinished(TSharedPtr<FEnvQueryResult> Result)
{
	PendingQueryID = INDEX_NONE;

	UBehaviorTreeComponent* Owner = CachedOwnerComp.Get();
	if (!Owner) return;

	UBlackboardComponent* BB = Owner->GetBlackboardComponent();
	if (!BB) return;

	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
	{
		BB->ClearValue(EnemyBBKeys::AttackPosition);
		return;
	}

	BB->SetValueAsVector(EnemyBBKeys::AttackPosition, Result->GetItemAsLocation(0));
}

void UBTService_SelectAbility::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
    if (!AIC) return;

    AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
    if (!Enemy) return;

    UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
    if (!ASC) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
    if (!Target) return;

    const float DistToTarget = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
    UObject* LastUsed = BB->GetValueAsObject(EnemyBBKeys::LastUsedAbility);
    FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
    float DotResult = FVector::DotProduct(Enemy->GetActorForwardVector(), ToTarget);
    float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotResult, -1.f, 1.f)));

    UGYEnemyAttackAbilityBase* BestAbility = nullptr;
    float BestScore = -1.f;

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        UGYEnemyAttackAbilityBase* Ability = Cast<UGYEnemyAttackAbilityBase>(Spec.Ability);
        if (!Ability) continue;

        float Score = UGYEnemyAttackAbilityBase::CalcAbilityScore(
            Ability, ASC, DistToTarget, AngleDeg, LastUsed);

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAbility = Ability;
        }
    }

    if (!BestAbility || !BestAbility->EQSAsset) return;

	FEnvQueryRequest QueryRequest(BestAbility->EQSAsset, Enemy);
	if (BestAbility->AttackType == EGYEnemyAttackType::Melee)
	{
		QueryRequest.SetFloatParam(TEXT("AttackRange"),    BestAbility->AttackRange * 0.7f);
		QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), BestAbility->AttackRange * 0.3f);
	}
	else
	{
		QueryRequest.SetFloatParam(TEXT("AttackRange"),    BestAbility->AttackRange * 0.95f);
		QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), BestAbility->AttackRange * 0.5f);
	}

	CachedOwnerComp = &OwnerComp;
	PendingQueryID = QueryRequest.Execute(
		EEnvQueryRunMode::SingleResult,
		this,
		&UBTService_SelectAbility::OnEQSFinished);
}
