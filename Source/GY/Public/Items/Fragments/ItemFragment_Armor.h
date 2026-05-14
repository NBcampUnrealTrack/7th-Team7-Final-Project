#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Armor.generated.h"

class USkeletalMesh;

UCLASS()
class GY_API UItemFragment_Armor : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ArmorAffinityTag;          // STR/DEX/INT 친화 (Mastery.STR 등)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<USkeletalMesh> OutfitMesh;
};
