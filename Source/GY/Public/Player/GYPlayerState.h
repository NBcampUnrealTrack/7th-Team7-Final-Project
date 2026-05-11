#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GYPlayerState.generated.h"

class UInventoryComponent;

UCLASS()
class GY_API AGYPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AGYPlayerState();

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInventoryComponent> InventoryComponent;
};
