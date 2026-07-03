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

	const float DistToTarget = FVector::Dist(
		Enemy->GetActorLocation(), Target->GetActorLocation());

	UGYEnemyAttackAbilityBase* BestAbility = nullptr;
	FGameplayAbilitySpecHandle BestHandle;
	float BestScore = -1.f;

	FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
	float DotResult = FVector::DotProduct(Enemy->GetActorForwardVector(), ToTarget);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotResult, -1.f, 1.f)));

	UObject* LastUsed = BlackboardComponent
	->GetValueAsObject(EnemyBBKeys::LastUsedAbility);

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGYEnemyAttackAbilityBase* AttackAbility =
			Cast<UGYEnemyAttackAbilityBase>(Spec.Ability);
		if (!AttackAbility) continue;

		float Score = UGYEnemyAttackAbilityBase::CalcAbilityScore(
			AttackAbility, ASC, Enemy, Target, LastUsed);

		if (Score < 0.f) continue;;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAbility = AttackAbility;
			BestHandle = Spec.Handle;
		}
	}

	if (!BestAbility) return;



	BlackboardComponent->SetValueAsObject(EnemyBBKeys::SelectedAbility, BestAbility);
	BlackboardComponent->SetValueAsObject(EnemyBBKeys::LastUsedAbility, BestAbility);
	return;


}

