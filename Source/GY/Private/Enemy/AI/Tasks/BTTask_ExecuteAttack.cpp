#include "Enemy/AI/Tasks/BTTask_ExecuteAttack.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ExecuteAttack::UBTTask_ExecuteAttack()
{
	NodeName = TEXT("Execute Ability");
	bNotifyTaskFinished = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ExecuteAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()
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

	UObject* LastUsed = OwnerComp.GetBlackboardComponent()
	->GetValueAsObject(EnemyBBKeys::LastUsedAbility);

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGYEnemyAttackAbilityBase* AttackAbility =
			Cast<UGYEnemyAttackAbilityBase>(Spec.Ability);
		if (!AttackAbility) continue;

		float Score = UGYEnemyAttackAbilityBase::CalcAbilityScore(
			AttackAbility, ASC, DistToTarget, AngleDeg, LastUsed);

		if (Score < 0.f) continue;;

		if (DistToTarget > AttackAbility->AttackRange + 20.f) continue;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAbility = AttackAbility;
			BestHandle = Spec.Handle;
		}
	}

	if (!BestAbility) return EBTNodeResult::Failed;

	Enemy->FaceToTarget(Target);

	CachedOwnerComp = &OwnerComp;
	ActiveAbility = BestAbility;
	CachedAbilityHandle = BestHandle;
	CachedASC = ASC;

	ASC->OnAbilityEnded.AddUObject(this, &UBTTask_ExecuteAttack::OnASCAbilityEnded);

	if (!ASC->TryActivateAbility(BestHandle))
	{
		ASC->OnAbilityEnded.RemoveAll(this);
		ActiveAbility = nullptr;
		CachedOwnerComp = nullptr;
		CachedASC = nullptr;
		return EBTNodeResult::Failed;
	}

	OwnerComp.GetBlackboardComponent()->SetValueAsObject(
		EnemyBBKeys::LastUsedAbility, BestAbility);

	return EBTNodeResult::InProgress;
}

void UBTTask_ExecuteAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	if (CachedASC)
	{
		CachedASC->OnAbilityEnded.RemoveAll(this);
		CachedASC = nullptr;
	}
	ActiveAbility = nullptr;

	if (TaskResult == EBTNodeResult::Aborted)
	{
		AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
		if (!AIC) return;

		AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
		if (!Enemy) return;

		if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
		{
			ASC->CancelAllAbilities();
		}
	}

	CachedOwnerComp = nullptr;
}

void UBTTask_ExecuteAttack::OnASCAbilityEnded(const FAbilityEndedData& EndedData)
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

