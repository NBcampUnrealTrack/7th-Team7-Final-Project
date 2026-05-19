#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
#include "Net/UnrealNetwork.h"

UGYWeaponAttribute::UGYWeaponAttribute()
{
	InitSwordAndShieldMultiplier(0.f);
}

void UGYWeaponAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYWeaponAttribute, SwordAndShieldMultiplier);
}

void UGYWeaponAttribute::OnRep_SwordAndShieldMultiplier(const FGameplayAttributeData& OldSwordAndShieldMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYWeaponAttribute, SwordAndShieldMultiplier, OldSwordAndShieldMultiplier);
}
