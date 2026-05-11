#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_PickupVisual.generated.h"

class UStaticMesh;

UCLASS()
class GY_API UItemFragment_PickupVisual : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UStaticMesh> PickupMesh;
};
