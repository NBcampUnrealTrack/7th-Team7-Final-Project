#include "Camera/CameraEffect_Zoom.h"

#include "Camera/GYCameraComponent.h"
#include "Logging/GYLogManager.h"

void UCameraEffect_Zoom::UpdateEffect(float DeltaTime, FGYCameraView& InOutView)
{
	Super::UpdateEffect(DeltaTime, InOutView);

	const float Duration = FMath::Max(Context.Duration, 0.01f);

	const float Alpha =
		FMath::Clamp(
			ElapsedTime / Duration,
			0.f,
			1.f);

	const float ZoomAlpha = FMath::Sin(Alpha * PI);

	const float Offset = Context.ZoomAmount * ZoomAlpha;

	InOutView.TargetArmLength -= Offset;
	GY_WARN(Player, CYS, "Zoom Offset = %.2f", Offset);

	// WorldLocation 방향으로 눌리는 효과
	if (Context.WorldLocation != FVector::ZeroVector)
	{
		// Pivot -> Target 방향
		const FVector Dir =
			(Context.WorldLocation - InOutView.PivotLocation).GetSafeNormal();

		// 얼마나 밀릴지
		const float PushAmount = Context.Intensity * ZoomAlpha;

		InOutView.PivotLocation += Dir * PushAmount;
		GY_WARN(Player, CYS, "Zoom 위치로 눌림 ");
	}
}
