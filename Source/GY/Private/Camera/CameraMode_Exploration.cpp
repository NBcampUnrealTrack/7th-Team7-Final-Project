#include "Camera/CameraMode_Exploration.h"
#include "Camera/GYCameraComponent.h"
#include "Camera/GYCameraModeData.h"

void UCameraMode_Exploration::UpdateCamera(float DeltaTime, FGYCameraView& OutView)
{
	Super::UpdateCamera(DeltaTime, OutView);

	APawn* Pawn = GetPawn();

	if (!Pawn || !CameraData)
	{
		return;
	}

	// 플레이어 위치
	FVector PlayerLocation = Pawn->GetActorLocation();

	// 최종 Pivot
	OutView.PivotLocation = PlayerLocation + CameraData->SocketOffset;
}
