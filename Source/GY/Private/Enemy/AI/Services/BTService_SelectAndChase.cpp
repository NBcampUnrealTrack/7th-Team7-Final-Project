#include "Enemy/AI/Services/BTService_SelectAndChase.h"

#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTService_SelectAndChase::UBTService_SelectAndChase()
{
	NodeName = TEXT("Select And Chase");
	Interval = 1.0f;
	RandomDeviation = 0.1f;
}

void UBTService_SelectAndChase::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (bQueryInProgress) return;

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return;

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()
		->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return;

	AIC->SetFocus(Target);

	const float DistToTarget = FVector::Dist(
		Enemy->GetActorLocation(), Target->GetActorLocation());

	UGYEnemyAttackAbilityBase* BestAbility = nullptr;
	float BestScore = -1.f;

	UObject* LastUsed = OwnerComp.GetBlackboardComponent()->GetValueAsObject(EnemyBBKeys::LastUsedAbility);

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGYEnemyAttackAbilityBase* AttackAbility = Cast<UGYEnemyAttackAbilityBase>(Spec.Ability);
		if (!AttackAbility) continue;

		if (AttackAbility->bHasCooldown && AttackAbility->GetRemainingCooldown(ASC) > 0.f) continue;

		float DamageScore = AttackAbility->GetTotalDamageScore();
		float ExtraMove = FMath::Max(0.f, DistToTarget - AttackAbility->AttackRange);
		float EffectiveScore = DamageScore / (1.f + ExtraMove * 0.01f);

		if (AttackAbility == LastUsed)
			EffectiveScore *= 0.3f;

		if (EffectiveScore > BestScore)
		{
			BestScore = EffectiveScore;
			BestAbility = AttackAbility;
		}
	}

	if (!BestAbility) return;

	if (!EQSAsset) return;

	UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(Enemy->GetWorld());
	if (!EQSManager) return;

	CachedOwnerComp = &OwnerComp;
	bQueryInProgress = true;

	FEnvQueryRequest QueryRequest(EQSAsset, Enemy);
	QueryRequest.SetFloatParam(TEXT("AttackRange"), BestAbility->AttackRange * 0.9f);
	QueryRequest.SetFloatParam(TEXT("AttackRangeMin"), BestAbility->AttackRange * 0.7f);
	QueryRequest.Execute(EEnvQueryRunMode::SingleResult, this, &UBTService_SelectAndChase::OnEQSFinished);
}

void UBTService_SelectAndChase::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
	bQueryInProgress = false;
	CachedOwnerComp = nullptr;

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIC)
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
}

void UBTService_SelectAndChase::OnEQSFinished(TSharedPtr<FEnvQueryResult> Result)
{
	bQueryInProgress = false;
	if (!Result.IsValid() || Result->IsAborted()) return;
	if (!CachedOwnerComp) return;

	FVector BestLocation = Result->GetItemAsLocation(0);

	CachedOwnerComp->GetBlackboardComponent()->SetValueAsVector(
		TEXT("AttackPosition"), BestLocation);
}
