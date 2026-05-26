#pragma once

#include "CoreMinimal.h"
#include "GYCameraModeBase.h"
#include "CameraMode_Combat.generated.h"

UCLASS()
class GY_API UCameraMode_Combat : public UGYCameraModeBase
{
	GENERATED_BODY()

public:
	virtual void ExitMode() override;
	virtual void UpdateCamera(float DeltaTime, FGYCameraView& OutView) override;
};
