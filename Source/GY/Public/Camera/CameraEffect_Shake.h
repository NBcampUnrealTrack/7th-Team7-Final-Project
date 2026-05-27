#pragma once

#include "CoreMinimal.h"
#include "GYCameraEffectBase.h"
#include "CameraEffect_Shake.generated.h"

UCLASS()
class GY_API UCameraEffect_Shake : public UGYCameraEffectBase
{
	GENERATED_BODY()

public:
	virtual void UpdateEffect(float DeltaTime, FGYCameraView& InOutView) override;
};
