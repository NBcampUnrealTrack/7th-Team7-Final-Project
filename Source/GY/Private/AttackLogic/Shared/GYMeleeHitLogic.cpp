#include "AttackLogic/Shared/GYMeleeHitLogic.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Core/GameplayTags/EventTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

void UGYMeleeHitLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
}

void UGYMeleeHitLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYMeleeHitLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Anim_Attack_DoTrace };
}

void UGYMeleeHitLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;
	if (!CachedAbility->GetCurrentActorInfo()->IsNetAuthority()) return;

	const AActor* TargetActor = Payload.Target.Get();
	if (!TargetActor) return;

	float Damage = 0.f;
	if (UAbilitySystemComponent* InstigatorASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		if (const UGYBaseAttribute* Attrs = InstigatorASC->GetSet<UGYBaseAttribute>())
		{
			Damage = Attrs->GetAttack() * CachedAbility->GetDamageMultiplier();
		}
	}

	if (const IAbilitySystemInterface* ASCInterface = Cast<const IAbilitySystemInterface>(TargetActor))
	{
		if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
		{
			UGYCombatStatics::ApplyDamage(TargetASC, Damage);
		}
	}
}
