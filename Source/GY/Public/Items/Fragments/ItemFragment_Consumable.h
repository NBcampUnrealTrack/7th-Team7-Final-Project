#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Consumable.generated.h"

class UGameplayEffect;

UCLASS()
class GY_API UItemFragment_Consumable : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Consumable.ChargePool"))
	FGameplayTag ChargePoolTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 ChargeCost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 StackCost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 MaxCharge = 5;
};
