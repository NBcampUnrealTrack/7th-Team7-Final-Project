#include "Enemy/AI/Services/BTService_SelectAbility.h"

#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"


UBTService_SelectAbility::UBTService_SelectAbility()
{
	NodeName = TEXT("Select Ability");
	Interval = 0.5f;
	bCallTickOnSearchStart = true;
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

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (BlackboardComponent->GetValueAsObject(EnemyBBKeys::SelectedAbility))
	{
		return;
	}

	AActor* Target = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return;

	UObject* LastUsed = BlackboardComponent
	->GetValueAsObject(EnemyBBKeys::LastUsedAbility);

	struct FAbilityScore
	{
		UGYEnemyAttackAbilityBase* Ability;
		float Score;
	};

	TArray<FAbilityScore> Candidates;
	float AccScores = 0.f;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGYEnemyAttackAbilityBase* AttackAbility = Cast<UGYEnemyAttackAbilityBase>(Spec.Ability);
		if (!AttackAbility) continue;

		float Score = UGYEnemyAttackAbilityBase::CalcAbilityScore(
			AttackAbility, ASC, Enemy, Target, LastUsed);

		if (Score < 0.f) continue;;

		Candidates.Add({AttackAbility, Score});
		AccScores += Score;
	}

	if (Candidates.Num() == 0 || AccScores <= 0.f) return;

	const float Roll = FMath::FRandRange(0.f, AccScores);
	float Cumulative = 0.f;

	UGYEnemyAttackAbilityBase* SelectedAbility = nullptr;

	for (const FAbilityScore& C : Candidates)
	{
		Cumulative += C.Score;
		if (Roll <= Cumulative)
		{
			SelectedAbility = C.Ability;
			break;
		}
	}

	if (!SelectedAbility) return;

	BlackboardComponent->SetValueAsObject(EnemyBBKeys::SelectedAbility, SelectedAbility);
	BlackboardComponent->SetValueAsObject(EnemyBBKeys::LastUsedAbility, SelectedAbility);
	return;
}

