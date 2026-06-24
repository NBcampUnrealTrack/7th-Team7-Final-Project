#include "Enemy/AI/StateTree/PatternEvaluator.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeLinker.h"
#include "Enemy/Component/EnemyAggroComponent.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"

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

static bool IsBossInCrowdControl(const UBossPatternSelectorComponent* Selector)
{
	if (!Selector) return false;
	const AAIController* AI = Cast<AAIController>(Selector->GetOwner());
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC) return false;

	static const FGameplayTag StaggerTag = FGameplayTag::RequestGameplayTag(TEXT("State.Hit.Stagger"));
	static const FGameplayTag StunTag = FGameplayTag::RequestGameplayTag(TEXT("State.Hit.Stun"));

	return ASC->HasMatchingGameplayTag(StaggerTag) || ASC->HasMatchingGameplayTag(StunTag);
}

void FPatternEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	UEnemyAggroComponent* Aggro = Context.GetExternalDataPtr(AggroHandle);

	if (!Selector || !Aggro)
	{
		Data.bHasReadyPattern = false;
		Data.bHasPendingAbility = false;
		return;
	}

	AActor* Target = Aggro->GetCurrentTarget();

	const bool bInCC = IsBossInCrowdControl(Selector);

	if (Target &&  !bInCC && Selector->GetPendingAbility() == nullptr)
	{
		Selector->SelectNextPattern(Target);
	}

	Data.bHasPendingAbility = (Selector->GetPendingAbility() != nullptr) && !bInCC;

	if (!Target)
	{
		Data.bHasReadyPattern = false;
		return;
	}

	Data.bHasReadyPattern = Selector->HasReadyPattern(Target) && !bInCC;
}
