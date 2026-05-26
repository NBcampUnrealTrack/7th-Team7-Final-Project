#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LootTypes.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct GY_API FLootDrop
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UItemDefinition> Definition;

	UPROPERTY(BlueprintReadOnly)
	int32 Count = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 EnhancementLevel = 0;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag GradeTag;

	UPROPERTY(BlueprintReadOnly)
	TArray<FName> RolledOptionIds;

	UPROPERTY(BlueprintReadOnly)
	float StatDeviation = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 UsedSeed = 0;
};

USTRUCT(BlueprintType)
struct GY_API FLootResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FLootDrop> Drops;
};

USTRUCT(BlueprintType)
struct GY_API FLootContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PartySize = 1;
};
