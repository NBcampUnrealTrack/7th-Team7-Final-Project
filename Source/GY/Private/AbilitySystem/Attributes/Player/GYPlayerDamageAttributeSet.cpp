#include "AbilitySystem/Attributes/Player/GYPlayerDamageAttributeSet.h"

UGYPlayerDamageAttributeSet::UGYPlayerDamageAttributeSet()
{
	InitAttack(10.f);
	InitDefense(5.f);
	InitCriticalRate(0.05f);
	InitCriticalMultiplier(1.5f);
}
