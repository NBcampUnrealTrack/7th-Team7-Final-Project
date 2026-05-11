#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "EnchantOptionRow.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct GY_API FEnchantOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> TemplateGE;

	UPROPERTY(EditAnywhere)
	float Magnitude1 = 0.f;

	UPROPERTY(EditAnywhere)
	float Magnitude2 = 0.f;

	UPROPERTY(EditAnywhere)
	float Magnitude3 = 0.f;

	UPROPERTY(EditAnywhere)
	FGameplayTag AffinityTag;

	UPROPERTY(EditAnywhere)
	FGameplayTag TriggerCondition;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 RollWeight = 1;

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere, meta = (MultiLine = true))
	FText Description;
};
