#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/DirectionTags.h"

UGYDodgeMontageFragment::UGYDodgeMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_DodgeMontage;
}

const FGYDodgeMontageSet* UGYDodgeMontageFragment::GetMontageForWeaponAndDirection(
	const FGameplayTagContainer& OwnedTags,
	FGameplayTag DirectionTag) const
{
	const FGYDodgeWeaponMontageSet* WeaponSet = nullptr;
	const FGYDodgeWeaponMontageSet* DefaultWeaponSet = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			DefaultWeaponSet = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			WeaponSet = &Pair.Value;
			break;
		}
	}

	if (!WeaponSet)
	{
		WeaponSet = DefaultWeaponSet;
	}

	if (!WeaponSet)
	{
		return nullptr;
	}

	if (const FGYDodgeMontageSet* Found = WeaponSet->DirectionMontages.Find(DirectionTag))
	{
		return Found;
	}

	if (const FGYDodgeMontageSet* Forward = WeaponSet->DirectionMontages.Find(GYGameplayTags::Direction_Forward))
	{
		return Forward;
	}

	if (!WeaponSet->DirectionMontages.IsEmpty())
	{
		return &WeaponSet->DirectionMontages.CreateConstIterator()->Value;
	}

	return nullptr;
}
