#include "Enemy/AI/Tasks/BTTask_SelectAbility.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQuery.h"

UBTTask_SelectAbility::UBTTask_SelectAbility()
{
	NodeName = TEXT("Execute Ability");
	bNotifyTaskFinished = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_SelectAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(BlackboardComponent
		->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return EBTNodeResult::Failed;

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
			AttackAbility, ASC, DistToTarget, AngleDeg, LastUsed);

		if (Score < 0.f) continue;;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAbility = AttackAbility;
			BestHandle = Spec.Handle;
		}
	}

	if (!BestAbility) return EBTNodeResult::Failed;

	CachedOwnerComp = &OwnerComp;
	ActiveAbility = BestAbility;
	CachedAbilityHandle = BestHandle;
	CachedASC = ASC;

	ASC->OnAbilityEnded.AddUObject(this, &UBTTask_SelectAbility::OnASCAbilityEnded);

	BlackboardComponent->SetValueAsFloat(EnemyBBKeys::AttackRadius,BestAbility->AttackRange * 0.75f);
	BlackboardComponent->SetValueAsFloat(EnemyBBKeys::AttackRadiusMin,BestAbility->MinDistance * 1.25f);
	BlackboardComponent->SetValueAsObject(EnemyBBKeys::EnvQuery, BestAbility->EQSAsset);
	BlackboardComponent->SetValueAsFloat(EnemyBBKeys::AttackAngle, BestAbility->AttackAngle);
	BlackboardComponent->SetValueAsObject(EnemyBBKeys::SelectedAbility, BestAbility);
	BlackboardComponent->SetValueAsObject(EnemyBBKeys::LastUsedAbility, BestAbility);
	return EBTNodeResult::Succeeded;
}

void UBTTask_SelectAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	if (CachedASC)
	{
		CachedASC->OnAbilityEnded.RemoveAll(this);
		CachedASC = nullptr;
	}
	ActiveAbility = nullptr;

	CachedOwnerComp = nullptr;
}

void UBTTask_SelectAbility::OnASCAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilitySpecHandle != CachedAbilityHandle) return;

	if (CachedASC)
	{
		CachedASC->OnAbilityEnded.RemoveAll(this);
	}

	if (!CachedOwnerComp)
	{
		return;
	}

	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}

