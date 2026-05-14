#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_WeaponAnimations.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;

UCLASS()
class GY_API UItemFragment_WeaponAnimations : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<UAnimInstance> AnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, TSoftObjectPtr<UAnimMontage>> ActionMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName AttachSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;
};
