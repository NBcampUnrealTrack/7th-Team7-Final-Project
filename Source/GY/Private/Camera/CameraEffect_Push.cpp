#include "Camera/CameraEffect_Push.h"

#include "Camera/GYCameraComponent.h"
#include "Logging/GYLogManager.h"

void UCameraEffect_Push::UpdateEffect(float DeltaTime, FGYCameraView& InOutView)
{
	Super::UpdateEffect(DeltaTime, InOutView);

	const float Duration = FMath::Max(Context.Duration, 0.01f);

	const float Alpha =
		FMath::Clamp(
			ElapsedTime / Duration,
			0.f,
			1.f);

	const float PushAlpha =
		FMath::Sin(Alpha * PI);

	const FVector Offset =
		Context.Direction.GetSafeNormal()
		* Context.Intensity
		* PushAlpha;

	InOutView.SocketOffset += Offset;
	GY_WARN(Player, CYS, "Push Offset = %s", *Offset.ToString());
}
