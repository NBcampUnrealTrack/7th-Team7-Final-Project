#include "Camera/GYCameraModeBase.h"
#include "Camera/GYCameraComponent.h"
#include "Camera/GYCameraModeData.h"
#include "GameFramework/Pawn.h"

void UGYCameraModeBase::Initialize(
	UGYCameraComponent* InCameraComponent,
	UGYCameraModeData* InCameraData)
{
	CameraComponent = InCameraComponent;
	CameraData = InCameraData;
}

void UGYCameraModeBase::EnterMode()
{
}

void UGYCameraModeBase::ExitMode()
{
}

void UGYCameraModeBase::UpdateCamera(
	float DeltaTime,
	FGYCameraView& OutView)
{
	if (!CameraData)
	{
		return;
	}

	APawn* Pawn = GetPawn();

	if (!Pawn)
	{
		return;
	}

	OutView.PivotLocation =
		Pawn->GetActorLocation();

	OutView.TargetArmLength = CameraData->TargetArmLength;

	OutView.FOV = CameraData->FOV;

	OutView.SocketOffset = CameraData->SocketOffset;

	OutView.LocationInterpSpeed = CameraData->LocationInterpSpeed;

	OutView.ZoomInterpSpeed = CameraData->ZoomInterpSpeed;

	OutView.FOVInterpSpeed = CameraData->FOVInterpSpeed;

	OutView.TargetArmRotation = CameraData->TargetArmRotation;

	OutView.RotationInterpSpeed = CameraData->RotationInterpSpeed;
}

APawn* UGYCameraModeBase::GetPawn() const
{
	if (!CameraComponent)
	{
		return nullptr;
	}

	return CameraComponent->GetPawn<APawn>();
}
