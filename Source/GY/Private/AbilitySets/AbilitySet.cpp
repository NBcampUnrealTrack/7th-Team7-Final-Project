#include "AbilitySets/AbilitySet.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

void UAbilitySet::GiveToAbilitySystem(
	UAbilitySystemComponent* ASC,
	FAbilitySetGrantedHandles* OutHandles,
	UObject* SourceObject) const
{
	if (!::IsValid(ASC)) return;
	if (!ASC->IsOwnerActorAuthoritative()) return;

	for (const FAbilitySet_GameplayAbility& AbilityEntry : GrantedAbilities)
	{
		if (!::IsValid(AbilityEntry.Ability)) continue;

		UGameplayAbility* AbilityCDO = AbilityEntry.Ability->GetDefaultObject<UGameplayAbility>();

		FGameplayAbilitySpec Spec(AbilityCDO, AbilityEntry.AbilityLevel);
		Spec.SourceObject = SourceObject;
		Spec.GetDynamicSpecSourceTags().AddTag(AbilityEntry.InputTag);

		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

		if (OutHandles != nullptr)
		{
			OutHandles->AbilitySpecHandles.Add(Handle);
		}
	}

	for (const FAbilitySet_GameplayEffect& EffectEntry : GrantedEffects)
	{
		if (!::IsValid(EffectEntry.GameplayEffect)) continue;

		const UGameplayEffect* EffectCDO = EffectEntry.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
			EffectCDO,
			EffectEntry.EffectLevel,
			ASC->MakeEffectContext());

		if (OutHandles != nullptr)
		{
			OutHandles->GameplayEffectHandles.Add(Handle);
		}
	}

	for (const FAbilitySet_AttributeSet& AttrEntry : GrantedAttributes)
	{
		if (!::IsValid(AttrEntry.AttributeSet)) continue;

		UAttributeSet* NewSet = NewObject<UAttributeSet>(ASC->GetOwner(), AttrEntry.AttributeSet);
		ASC->AddAttributeSetSubobject(NewSet);

		if (OutHandles != nullptr)
		{
			OutHandles->GrantedAttributeSets.Add(NewSet);
		}
	}
}

void FAbilitySetGrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	if (!::IsValid(ASC)) return;
	if (!ASC->IsOwnerActorAuthoritative()) return;

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		if (::IsValid(Set))
		{
			ASC->RemoveSpawnedAttribute(Set);
		}
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

bool FAbilitySetGrantedHandles::IsValid() const
{
	return AbilitySpecHandles.Num() > 0
		|| GameplayEffectHandles.Num() > 0
		|| GrantedAttributeSets.Num() > 0;
}
