#include "Core/GameplayCue/GYGameplayCueNotify_Camera.h"

#include "Camera/GYCameraComponent.h"
#include "Logging/GYLogManager.h"

/* 필요한 큐 파라미터 */
/* 대상: Parameters.Instigator
 * 위치: Parameters.Location
 * 방향: Parameters.Normal
 */

bool UGYGameplayCueNotify_Camera::OnExecute_Implementation(AActor* MyTarget,
                                                           const FGameplayCueParameters& Parameters) const
{
	GY_WARN(Player, CYS, "카메라 게임플레이큐 실행");
	APawn* Pawn =
		Cast<APawn>(MyTarget);

	if (!Pawn)
	{
		return false;
	}

	if (!Pawn->IsLocallyControlled())
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
	case EGYCameraDirectionSource::HitNormal: //파라미터 노멀값 기준
		Context.Direction = Parameters.Normal;
		break;
	case EGYCameraDirectionSource::Instigator: //대상이 바라보는 방향
		if (Parameters.Instigator.IsValid())
		{
			Context.Direction =
				Parameters.Instigator->GetActorForwardVector();
		}
		else
		{
			Context.Direction = FVector::ZeroVector;
		}
		break;
	default:
		Context.Direction = FVector::ZeroVector;
		break;
	}

	Context.WorldLocation = Parameters.Location;

	CameraComp->PushCameraEffect(Context);

	return true;
}
