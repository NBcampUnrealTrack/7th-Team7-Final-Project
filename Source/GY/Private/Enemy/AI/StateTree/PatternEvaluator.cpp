#include "Enemy/AI/StateTree/PatternEvaluator.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeLinker.h"
#include "Enemy/Component/EnemyAggroComponent.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "Logging/GYLogManager.h"

bool FPatternEvaluator::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(SelectorHandle);
	Linker.LinkExternalData(AggroHandle);
	return true;
}

void FPatternEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bHasReadyPattern = false;
	Data.bHasPendingAbility = false;
}

static bool IsBossUnavailableForPattern(const UBossPatternSelectorComponent* Selector)
{
	if (!Selector) return false;
	const AAIController* AI = Cast<AAIController>(Selector->GetOwner());
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC) return false;

	static const FGameplayTag StaggerTag = FGameplayTag::RequestGameplayTag(TEXT("State.Hit.Stagger"));
	static const FGameplayTag StunTag = FGameplayTag::RequestGameplayTag(TEXT("State.Hit.Stun"));
	static const FGameplayTag AttackOwnedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Enemy"));

	return ASC->HasMatchingGameplayTag(StaggerTag)
		|| ASC->HasMatchingGameplayTag(StunTag)
		|| ASC->HasMatchingGameplayTag(AttackOwnedTag);
}

void FPatternEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	UEnemyAggroComponent* Aggro = Context.GetExternalDataPtr(AggroHandle);

	if (!Selector || !Aggro)
	{
		if (Data.bHasReadyPattern || Data.bHasPendingAbility)
		{
			GY_WARN(AI, ESK, "PatternEval: 의존 컴포넌트 없음 Selector=%d Aggro=%d",
				Selector != nullptr, Aggro != nullptr);
		}
		Data.bHasReadyPattern = false;
		Data.bHasPendingAbility = false;
		return;
	}

	AActor* Target = Aggro->GetCurrentTarget();

	const bool bInCC = IsBossUnavailableForPattern(Selector);

	// Select는 SelectPatternTask에서 명시적으로 수행. Evaluator는 평가만.
	const bool bNewPending = (Selector->GetPendingAbility() != nullptr) && !bInCC;
	const bool bNewReady = (Target != nullptr) && !bInCC;

	if (bNewPending != Data.bHasPendingAbility || bNewReady != Data.bHasReadyPattern)
	{
		GY_LOG(AI, ESK,
			"PatternEval: 상태변경 Ready[%d→%d] Pending[%d→%d] Target=%s InCC=%d PendingAbility=%s",
			Data.bHasReadyPattern, bNewReady,
			Data.bHasPendingAbility, bNewPending,
			*GetNameSafe(Target),
			bInCC ? 1 : 0,
			*GetNameSafe(Selector->GetPendingAbility().Get()));
	}

	Data.bHasPendingAbility = bNewPending;
	Data.bHasReadyPattern = bNewReady;
}
