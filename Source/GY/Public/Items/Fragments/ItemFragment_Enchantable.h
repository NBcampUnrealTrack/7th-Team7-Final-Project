#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Enchantable.generated.h"

class UDataTable;

UCLASS()
class GY_API UItemFragment_Enchantable : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UDataTable> EnchantOptionPoolTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 MaxOptionSlots = 1;
};
