#pragma once

#include "CoreMinimal.h"
#include "GYCameraEffectTypes.h"
#include "UObject/Object.h"
#include "GYCameraEffectBase.generated.h"

class UGYCameraComponent;
class UGYCameraEffectData;
struct FGYCameraView;
struct FGYCameraEffectContext;

UCLASS(Abstract)
class GY_API UGYCameraEffectBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(const FGYCameraEffectContext& InContext);
	virtual void UpdateEffect(float DeltaTime, FGYCameraView& InOutView);

	virtual bool IsFinished() const;

protected:
	UPROPERTY()
	FGYCameraEffectContext Context;

	float ElapsedTime = 0.f;
};
