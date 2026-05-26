#pragma once

#include "CoreMinimal.h"
#include "GYCameraModeBase.h"
#include "CameraMode_Exploration.generated.h"

UCLASS()
class GY_API UCameraMode_Exploration : public UGYCameraModeBase
{
	GENERATED_BODY()

public:
	virtual void ExitMode() override;
	virtual void UpdateCamera(
		float DeltaTime,
		FGYCameraView& OutView) override;

private:
	AActor* FindTargetActor() const;

	TWeakObjectPtr<AActor> CachedTargetActor;

	UPROPERTY(EditDefaultsOnly, Category="Camera")
	FName TargetActorTag = FName("Target");
};
