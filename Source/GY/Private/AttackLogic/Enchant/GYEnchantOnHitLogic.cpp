#include "AttackLogic/Enchant/GYEnchantOnHitLogic.h"

#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYOnHitModifierComponent.h"
#include "Character/GYCharacter.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/OptionTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

void UGYEnchantOnHitLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
}

void UGYEnchantOnHitLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYEnchantOnHitLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Anim_Attack_DoTrace };
}

void UGYEnchantOnHitLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;
	if (!CachedAbility->GetCurrentActorInfo()->IsNetAuthority()) return;

	UAbilitySystemComponent* SourceASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	// 값: 공격자 OnHitModifier에서 이 효과의 매그니튜드 합산. 0이면(=해당 인첸트 미장착) 무시.
	const AGYCharacter* Character = CachedAbility->GetGYCharacter();
	const UGYOnHitModifierComponent* Modifiers = IsValid(Character) ? Character->GetOnHitModifierComponent() : nullptr;
	if (!IsValid(Modifiers)) return;

	const float Value = Modifiers->GetModifierSumValue(MagnitudeTag);
	if (FMath::IsNearlyZero(Value)) return;

	// 적용 대상: 출혈·방깎=피격 대상, 흡혈=공격자 자신.
	UAbilitySystemComponent* ApplyToASC = SourceASC;
	if (bApplyToTarget)
	{
		const AActor* TargetActor = Payload.Target.Get();
		const IAbilitySystemInterface* ASCInterface = Cast<const IAbilitySystemInterface>(TargetActor);
		ApplyToASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	}
	if (!ApplyToASC) return;

	TSubclassOf<UGameplayEffect> Effect = EffectClass.LoadSynchronous();
	if (!Effect) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Effect, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::Stat_Modifier_OptionMagnitude1, Value * ValueScale);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), ApplyToASC);
}
