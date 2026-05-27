#include "Core/GameplayCue/GYGameplayCueNotify_Camera.h"

#include "Camera/GYCameraComponent.h"
#include "Logging/GYLogManager.h"

bool UGYGameplayCueNotify_Camera::OnExecute_Implementation(AActor* MyTarget,
                                                           const FGameplayCueParameters& Parameters) const
{
	GY_WARN(Player,CYS,"카메라 게임플레이큐 실행");
	APawn* Pawn =
		Cast<APawn>(MyTarget);

	if (!Pawn)
	{
		return false;
	}

	UGYCameraComponent* CameraComp =
		Pawn->FindComponentByClass<
			UGYCameraComponent>();

	if (!CameraComp)
	{
		return false;
	}

	FGYCameraEffectContext Context;

	Context.Type = EffectType;
	Context.Intensity = Intensity;
	Context.Duration = Duration;
	Context.ZoomAmount = ZoomAmount;
	Context.Frequency = Frequency;
	// 방향 계산
	switch (DirectionSource)
	{
	case EGYCameraDirectionSource::HitNormal:
		Context.Direction = Parameters.Normal;
		break;
	case EGYCameraDirectionSource::Instigator:
		if (Parameters.Instigator.IsValid())
		{
			Context.Direction =
				Parameters.Instigator->GetActorForwardVector();
		}
		break;
	default:
		break;
	}

	Context.WorldLocation = Parameters.Location;

	CameraComp->PushCameraEffect(Context);

	return true;
}
