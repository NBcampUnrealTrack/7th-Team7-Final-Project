#include "Enemy/Actor/GYWeaponActor.h"
#include "Enemy/Actor/GYWeaponHitBox.h"


AGYWeaponActor::AGYWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HitBox = CreateDefaultSubobject<UGYWeaponHitBox>(TEXT("HitBox"));
	HitBox->SetupAttachment(WeaponMesh);
}


