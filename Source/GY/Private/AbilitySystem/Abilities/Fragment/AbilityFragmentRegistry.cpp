// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Fragment/AbilityFragmentRegistry.h"


TArray<TSubclassOf<UAbilityFragment>> UAbilityFragmentRegistry::GetRequiredFragmentClasses(FGameplayTag AbilityTag) const
{
	if (const FAbilityFragmentRequirements* Req = AbilitySchema.Find(AbilityTag))
		return Req->RequiredFragmentClasses;
	return {};
}
