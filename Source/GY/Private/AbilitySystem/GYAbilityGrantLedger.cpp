#include "AbilitySystem/GYAbilityGrantLedger.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

bool FGYAbilityGrantLedger::HasSource(FGameplayTag Source) const
{
	return Records.Contains(Source);
}

void FGYAbilityGrantLedger::AddLooseTag(UAbilitySystemComponent* ASC, FGameplayTag Source, FGameplayTag Tag,
	int32 Count, EGameplayTagReplicationState TagRepState)
{
	if (!IsValid(ASC) || !Tag.IsValid()) return;

	FGYAbilityGrantRecord& Record = Records.FindOrAdd(Source);

	const bool bAlreadyGranted = Record.LooseTags.ContainsByPredicate(
		[&Tag](const FGYGrantedLooseTag& Existing) { return Existing.Tag == Tag; });
	if (bAlreadyGranted) return;

	ASC->AddLooseGameplayTag(Tag, Count, TagRepState);

	FGYGrantedLooseTag& Granted = Record.LooseTags.AddDefaulted_GetRef();
	Granted.Tag = Tag;
	Granted.Count = Count;
	Granted.RepState = TagRepState;
}

FActiveGameplayEffectHandle FGYAbilityGrantLedger::ApplyEffectSpec(UAbilitySystemComponent* ASC, FGameplayTag Source,
	const FGameplayEffectSpec& Spec)
{
	if (!IsValid(ASC)) return FActiveGameplayEffectHandle();

	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
	if (Handle.IsValid())
	{
		Records.FindOrAdd(Source).EffectHandles.Add(Handle);
	}
	return Handle;
}

void FGYAbilityGrantLedger::AdoptHandles(FGameplayTag Source, const FAbilitySetGrantedHandles& Handles)
{
	if (!Handles.HasAnyHandles()) return;

	FGYAbilityGrantRecord& Record = Records.FindOrAdd(Source);
	Record.AbilitySpecHandles.Append(Handles.AbilitySpecHandles);
	Record.EffectHandles.Append(Handles.GameplayEffectHandles);
	for (UAttributeSet* Set : Handles.GrantedAttributeSets)
	{
		Record.AttributeSets.Add(Set);
	}
}

void FGYAbilityGrantLedger::RevokeSource(UAbilitySystemComponent* ASC, FGameplayTag Source)
{
	FGYAbilityGrantRecord* Record = Records.Find(Source);
	if (Record == nullptr) return;

	if (IsValid(ASC))
	{
		for (const FGYGrantedLooseTag& Granted : Record->LooseTags)
		{
			ASC->RemoveLooseGameplayTag(Granted.Tag, Granted.Count, Granted.RepState);
		}
		for (const FActiveGameplayEffectHandle& Handle : Record->EffectHandles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
		for (const FGameplayAbilitySpecHandle& Handle : Record->AbilitySpecHandles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}
		for (UAttributeSet* Set : Record->AttributeSets)
		{
			if (IsValid(Set))
			{
				ASC->RemoveSpawnedAttribute(Set);
			}
		}
	}

	Records.Remove(Source);
}
