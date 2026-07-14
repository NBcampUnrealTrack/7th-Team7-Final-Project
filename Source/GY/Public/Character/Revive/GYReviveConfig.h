#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "GYReviveConfig.generated.h"

class AGYDownedDecorationActor;
class UGameplayAbility;

UCLASS(BlueprintType)
class GY_API UGYReviveConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Revive", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RevivePoolRequiredPercent = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Revive", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ReviveHealthGrantedPercent = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Revive", meta=(ClampMin="0.001"))
	float ReviveCostRatePerSecond = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category="Revive", meta=(ClampMin="0.0"))
	float MinReviverHealthReserve = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Revive", meta=(ClampMin="0.1"))
	float GiveUpHoldTime = 3.f;

	UPROPERTY(EditDefaultsOnly, Category="Revive", meta=(ClampMin="0.0"))
	float RespawnDelayAfterGiveUp = 3.f;

	UPROPERTY(EditDefaultsOnly, Category="Tags")
	FGameplayTagContainer DownedTags;

	UPROPERTY(EditDefaultsOnly, Category="GiveUp")
	FGameplayTag GiveUpInputTag;

	UPROPERTY(EditDefaultsOnly, Category="Revive")
	FText ReviveInteractionText;

	UPROPERTY(EditDefaultsOnly, Category="Revive")
	TSubclassOf<UGameplayAbility> ReviveAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category="Revive")
	TSubclassOf<AGYDownedDecorationActor> DecorationActorClass;
};
