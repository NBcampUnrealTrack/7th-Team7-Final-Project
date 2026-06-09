#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GYPlayerActionConfig.generated.h"

UCLASS(BlueprintType)
class GY_API UGYPlayerActionConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Death")
	FGameplayTagContainer DeathTags;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	FGameplayTagContainer InputBlockTags;

	UPROPERTY(EditDefaultsOnly, Category="Respawn", meta=(ClampMin="0.0", Units="s"))
	float RespawnDelay = 5.f;
};
