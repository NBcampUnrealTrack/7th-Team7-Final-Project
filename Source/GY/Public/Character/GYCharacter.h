#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GYCharacter.generated.h"

class UEquipmentComponent;

UCLASS()
class GY_API AGYCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGYCharacter();

	UFUNCTION(BlueprintPure)
	UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEquipmentComponent> EquipmentComponent;
};
