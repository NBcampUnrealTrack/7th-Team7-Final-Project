#include "Equipment/GYEquipmentActor.h"

#include "Components/StaticMeshComponent.h"

AGYEquipmentActor::AGYEquipmentActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

void AGYEquipmentActor::SetWeaponMesh(UStaticMesh* NewMesh) const
{
	if (NewMesh == nullptr) return;

	WeaponMesh->SetStaticMesh(NewMesh);
}
