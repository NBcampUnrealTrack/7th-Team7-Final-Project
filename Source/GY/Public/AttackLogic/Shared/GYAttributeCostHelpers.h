#pragma once

#include "AttackLogic/Shared/GYAttributeCost.h"
#include "AbilitySystemComponent.h"

namespace GYAttributeCostHelpers
{
	inline void ApplyCost(UAbilitySystemComponent* ASC, const FGYAttributeCost& Cost)
	{
		if (!ASC || !Cost.Attribute.IsValid() || Cost.Amount <= 0.f) return;
		const float Current = ASC->GetNumericAttributeBase(Cost.Attribute);
		ASC->SetNumericAttributeBase(Cost.Attribute, FMath::Max(0.f, Current - Cost.Amount));
	}

	inline void ApplyReward(UAbilitySystemComponent* ASC, const FGYAttributeCost& Reward)
	{
		if (!ASC || !Reward.Attribute.IsValid() || Reward.Amount <= 0.f) return;
		const float Current = ASC->GetNumericAttributeBase(Reward.Attribute);
		ASC->SetNumericAttributeBase(Reward.Attribute, Current + Reward.Amount);
	}
}
