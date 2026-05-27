#pragma once

#include "CoreMinimal.h"
#include "GYCameraEffectBase.h"
#include "CameraEffect_Zoom.generated.h"

UCLASS()
class GY_API UCameraEffect_Zoom : public UGYCameraEffectBase
{
	GENERATED_BODY()

public:
	virtual void UpdateEffect(float DeltaTime, FGYCameraView& InOutView) override;
};
