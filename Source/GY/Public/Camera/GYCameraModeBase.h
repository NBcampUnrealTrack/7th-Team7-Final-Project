#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GYCameraModeBase.generated.h"

struct FGYCameraView;
class UGYCameraComponent;
class UGYCameraModeData;


UCLASS()
class GY_API UGYCameraModeBase : public UObject
{
	GENERATED_BODY()
public:

	virtual void Initialize(
		UGYCameraComponent* InCameraComponent,
		UGYCameraModeData* InCameraData);

	virtual void EnterMode();
	virtual void ExitMode();

	virtual void UpdateCamera(
		float DeltaTime,
		FGYCameraView& OutView);

	FORCEINLINE UGYCameraModeData* GetCameraData() const
	{
		return CameraData;
	}

protected:

	UPROPERTY(Transient)
	TObjectPtr<UGYCameraComponent> CameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGYCameraModeData> CameraData;

	float BlendAlpha = 0.f;
	float ElapsedTime = 0.f;

protected:

	APawn* GetPawn() const;
};
