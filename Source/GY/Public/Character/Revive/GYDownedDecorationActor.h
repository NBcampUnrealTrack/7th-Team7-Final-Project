#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GYDownedDecorationActor.generated.h"

UCLASS(Blueprintable)
class GY_API AGYDownedDecorationActor : public AActor
{
	GENERATED_BODY()

public:
	AGYDownedDecorationActor();

	UFUNCTION(BlueprintImplementableEvent, Category="Revive")
	void OnRevivePoolPercentChanged(float NewPercent);

	UFUNCTION(BlueprintImplementableEvent, Category="Revive")
	void OnReviveCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category="Revive")
	void OnReviveAbandoned();
};
