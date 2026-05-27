#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GYAnimInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGYAnimInterface : public UInterface
{
	GENERATED_BODY()
};

class GY_API IGYAnimInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Animation")
	void SetDodgeDirection(float Angle);
};
