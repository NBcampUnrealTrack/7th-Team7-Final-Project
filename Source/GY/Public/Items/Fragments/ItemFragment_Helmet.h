#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Helmet.generated.h"

class USkeletalMesh;

UCLASS()
class GY_API UItemFragment_Helmet : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<USkeletalMesh> HelmetMesh;
};
