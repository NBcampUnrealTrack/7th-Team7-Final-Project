#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "InteractionOption.generated.h"

class IInteractable;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct GY_API FInteractionOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FText Text;

	UPROPERTY(BlueprintReadWrite, meta = (Categories = "Interaction"))
	FGameplayTag OptionTag;

	UPROPERTY(BlueprintReadWrite)
	int32 Priority = 0;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> InteractionAbilityToGrant;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> SourceObject;
};
