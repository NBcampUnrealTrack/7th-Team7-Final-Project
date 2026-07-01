// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ParkourDodge/GYParkourDodgeRouterLogic.h"

#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"

void UGYParkourDodgeRouterLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	Super::OnExecute(Ability);
	CachedAbility = Ability;
	CachedParkourFragment = Ability->GetFragment<UGYParkourFragment>();

	// 로컬 컨트롤러에서만 환경을 판별하여 라우팅합니다.
	if (Ability->GetActorInfo().IsLocallyControlled())
	{
		DetermineActionAndRoute();
	}
}

void UGYParkourDodgeRouterLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	Super::OnAbilityEnd(Ability, bWasCancelled);
}

void UGYParkourDodgeRouterLogic::DetermineActionAndRoute()
{

}

bool UGYParkourDodgeRouterLogic::CheckParkourEnvironment()
{

	return true;
}
