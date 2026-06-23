#include "AttackLogic/Shared/GYMontageOverrideLogic.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AttackLogic/Combo/GYComboMontageFragment.h"
#include "AttackLogic/Combo/GYComboMontageOverrideFragment.h"
#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "AttackLogic/Charge/GYChargeMontageOverrideFragment.h"
#include "AttackLogic/Parry/GYParryMontageFragment.h"
#include "AttackLogic/Parry/GYParryMontageOverrideFragment.h"
#include "AttackLogic/Block/GYBlockMontageFragment.h"
#include "AttackLogic/Block/GYBlockMontageOverrideFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageOverrideFragment.h"

void UGYMontageOverrideLogic::OnPreExecute(UGYPlayerGameplayAbility* Ability)
{
	if (auto* Override = Ability->GetFragment<UGYComboMontageOverrideFragment>())
		if (auto* Target = Ability->GetFragment<UGYComboMontageFragment>())
			Target->MontageAnimSets = Override->MontageAnimSets;

	if (auto* Override = Ability->GetFragment<UGYChargeMontageOverrideFragment>())
		if (auto* Target = Ability->GetFragment<UGYChargeMontageFragment>())
			Target->MontageAnimSets = Override->MontageAnimSets;

	if (auto* Override = Ability->GetFragment<UGYParryMontageOverrideFragment>())
		if (auto* Target = Ability->GetFragment<UGYParryMontageFragment>())
			Target->MontageAnimSets = Override->MontageAnimSets;

	if (auto* Override = Ability->GetFragment<UGYBlockMontageOverrideFragment>())
		if (auto* Target = Ability->GetFragment<UGYBlockMontageFragment>())
			Target->MontageAnimSets = Override->MontageAnimSets;

	if (auto* Override = Ability->GetFragment<UGYDodgeMontageOverrideFragment>())
		if (auto* Target = Ability->GetFragment<UGYDodgeMontageFragment>())
			Target->MontageAnimSets = Override->MontageAnimSets;
}
