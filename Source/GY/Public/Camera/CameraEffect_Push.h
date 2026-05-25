#pragma once

#include "CoreMinimal.h"
#include "GYCameraEffectBase.h"
#include "CameraEffect_Push.generated.h"

UCLASS()
class GY_API UCameraEffect_Push : public UGYCameraEffectBase
{
	GENERATED_BODY()

public:
	virtual void UpdateEffect(float DeltaTime, FGYCameraView& InOutView) override;
};
