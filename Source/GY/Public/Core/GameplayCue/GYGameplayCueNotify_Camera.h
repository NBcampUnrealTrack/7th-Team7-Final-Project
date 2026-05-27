#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Camera/GYCameraEffectTypes.h"
#include "GYGameplayCueNotify_Camera.generated.h"

enum class EGYCameraEffectType : uint8;

UCLASS(Abstract, Blueprintable)
class GY_API UGYGameplayCueNotify_Camera : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGYCameraEffectType EffectType =
		EGYCameraEffectType::Shake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Intensity = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Duration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ZoomAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGYCameraDirectionSource DirectionSource =
		EGYCameraDirectionSource::HitNormal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Frequency = 40.f;
};
