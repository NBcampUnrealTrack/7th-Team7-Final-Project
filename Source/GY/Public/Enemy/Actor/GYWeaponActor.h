#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "GYWeaponActor.generated.h"

class UGYWeaponHitBox;

UCLASS()
class GY_API AGYWeaponActor : public AActor
{
	GENERATED_BODY()
public:
	AGYWeaponActor();

	UGYWeaponHitBox* GetHitBox() const { return HitBox; }
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FGameplayTag GetWeaponTypeTag() const { return WeaponTypeTag; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UGYWeaponHitBox> HitBox;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (Categories = "Weapon.Type"))
	FGameplayTag WeaponTypeTag;
};
