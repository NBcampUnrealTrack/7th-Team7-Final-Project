#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_QuestKey.generated.h"

UCLASS()
class GY_API UItemFragment_QuestKey : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag QuestTag;
};
