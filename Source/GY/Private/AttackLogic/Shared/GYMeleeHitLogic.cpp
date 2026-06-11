#include "AttackLogic/Shared/GYMeleeHitLogic.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
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

	// 1차 스탯(STR/DEX) 1포인트당 무기 데미지 +1.5% (둘 다 동일 기여)
	constexpr float StatToWeaponDamage = 0.015f;

	float Damage = 0.f;
	UAbilitySystemComponent* InstigatorASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (InstigatorASC)
	{
		if (const UGYDamageAttributeSet* Attrs = InstigatorASC->GetSet<UGYDamageAttributeSet>())
		{
			Damage = Attrs->GetAttack() * CachedAbility->GetDamageMultiplier();
		}
		if (const UGYCoreStatAttributeSet* CoreStat = InstigatorASC->GetSet<UGYCoreStatAttributeSet>())
		{
			const float StatBonus = (CoreStat->GetStrength() + CoreStat->GetDexterity()) * StatToWeaponDamage;
			Damage *= (1.f + StatBonus);
		}
	}

	if (const IAbilitySystemInterface* ASCInterface = Cast<const IAbilitySystemInterface>(TargetActor))
	{
		if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
		{
			UGYCombatStatics::ApplyDamage(TargetASC, Damage, InstigatorASC);
		}
	}
}
