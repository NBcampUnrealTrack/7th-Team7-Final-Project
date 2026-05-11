#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GYCharacter.generated.h"

class UActiveEquipmentComponent;

UCLASS()
class GY_API AGYCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGYCharacter();

	UFUNCTION(BlueprintPure)
	UActiveEquipmentComponent* GetActiveEquipmentComponent() const { return ActiveEquipmentComponent; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UActiveEquipmentComponent> ActiveEquipmentComponent;
};
