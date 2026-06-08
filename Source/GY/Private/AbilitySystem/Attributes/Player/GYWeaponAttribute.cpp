#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
#include "Net/UnrealNetwork.h"

UGYWeaponAttribute::UGYWeaponAttribute()
{
	InitWeaponDamageMultiplier(1.f);
}

void UGYWeaponAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYWeaponAttribute, WeaponDamageMultiplier);
}

void UGYWeaponAttribute::OnRep_WeaponDamageMultiplier(const FGameplayAttributeData& OldWeaponDamageMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYWeaponAttribute, WeaponDamageMultiplier, OldWeaponDamageMultiplier);
}
