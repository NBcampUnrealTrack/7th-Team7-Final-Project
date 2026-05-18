#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"

bool UGYEnemyAttackAbilityBase::CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const
{
	if (!ASC) return false;

	if (DistToTarget > AttackRange) return false;

	if (bHasCooldown && GetRemainingCooldown(ASC) > 0.f) return false;

	return true;
}

float UGYEnemyAttackAbilityBase::GetRemainingCooldown(const UAbilitySystemComponent* ASC) const
{
	if (!ASC || !bHasCooldown) return 0.f;

	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!CooldownTags || CooldownTags->IsEmpty()) return 0.f;

	FGameplayEffectQuery Query =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);

	TArray<float> Durations = ASC->GetActiveEffectsTimeRemaining(Query);
	if (Durations.IsEmpty()) return 0.f;

	return FMath::Max(0.f, Durations[0]);
}
