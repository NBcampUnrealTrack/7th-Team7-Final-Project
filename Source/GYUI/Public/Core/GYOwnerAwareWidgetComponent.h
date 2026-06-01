#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GYOwnerAwareWidgetComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GYUI_API UGYOwnerAwareWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

protected:
	virtual void InitWidget() override;
};
