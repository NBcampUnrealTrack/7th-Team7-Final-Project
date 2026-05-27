#include "Camera/CameraEffect_Shake.h"

#include "Camera/GYCameraComponent.h"
#include "Logging/GYLogManager.h"

void UCameraEffect_Shake::UpdateEffect(float DeltaTime, FGYCameraView& InOutView)
{
	Super::UpdateEffect(DeltaTime, InOutView);

	const float Duration = FMath::Max(Context.Duration, 0.01f);

	const float Alpha =
		FMath::Clamp(
			ElapsedTime / Duration,
			0.f,
			1.f);

	const float Fade = 1.f - Alpha;

	const FVector Dir =
		Context.Direction.GetSafeNormal();

	const FVector Perp =
		FVector::CrossProduct(
			Dir,
			FVector::UpVector).GetSafeNormal();

	// 메인 진동
	const float MainWave =
		FMath::Sin(
			ElapsedTime * Context.Frequency);

	// 보조 진동
	const float SubWave =
		FMath::Sin(
			ElapsedTime * Context.Frequency * 1.7f);

	// 방향 쉐이크
	const FVector MainShake =
		Dir
		* MainWave
		* Context.Intensity;

	// 수직 보조 쉐이크
	const FVector SubShake =
		Perp
		* SubWave
		* (Context.Intensity * 0.35f);

	InOutView.SocketOffset +=
		(MainShake + SubShake)
		* Fade;
	GY_WARN(Player, CYS, "Shake");
}
