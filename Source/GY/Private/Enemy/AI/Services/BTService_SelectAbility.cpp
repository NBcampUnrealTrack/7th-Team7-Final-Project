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
	RandomDeviation = 0.1f;
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

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return;

	const float DistToTarget = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
	UObject* LastUsed = BB->GetValueAsObject(EnemyBBKeys::LastUsedAbility);

	UGYEnemyAttackAbilityBase* BestAbility = nullptr;
	float BestScore = -1.f;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGYEnemyAttackAbilityBase* Ability = Cast<UGYEnemyAttackAbilityBase>(Spec.Ability);
		if (!Ability) continue;
		if (Ability->bHasCooldown && Ability->GetRemainingCooldown(ASC) > 0.f) continue;

		float Score = Ability->GetTotalDamageScore();
		float ExtraMove = FMath::Max(0.f, DistToTarget - Ability->AttackRange);
		float EffectiveScore = Score / (1.f + ExtraMove * 0.01f);

		if (Ability == LastUsed) EffectiveScore *= 0.3f;

		if (EffectiveScore > BestScore)
		{
			BestScore = EffectiveScore;
			BestAbility = Ability;
		}
	}

	BB->SetValueAsObject(EnemyBBKeys::SelectedAbility, BestAbility);
}
