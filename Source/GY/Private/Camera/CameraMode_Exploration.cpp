#include "Camera/CameraMode_Exploration.h"

#include "EngineUtils.h"
#include "Camera/GYCameraComponent.h"
#include "Camera/GYCameraModeData.h"

void UCameraMode_Exploration::ExitMode()
{
	Super::ExitMode();
	CachedTargetActor.Reset();
}

void UCameraMode_Exploration::UpdateCamera(float DeltaTime, FGYCameraView& OutView)
{
	Super::UpdateCamera(DeltaTime, OutView);

	APawn* Pawn = GetPawn();

	if (!Pawn || !CameraData)
	{
		return;
	}

	if (!CachedTargetActor.IsValid())
	{
		CachedTargetActor = FindTargetActor();
	}
	if (!CachedTargetActor.IsValid()) return;

	// 플레이어 위치
	const FVector PlayerLoc = Pawn->GetActorLocation();
	const FVector TargetLoc = CachedTargetActor->GetActorLocation();
	const float Distance = FVector::Dist(PlayerLoc, TargetLoc);

	// 최종 Pivot
	OutView.PivotLocation = FMath::Lerp(PlayerLoc, TargetLoc, CameraData->PlayerWeight);

	// 거리에 따라 줌 인
	OutView.TargetArmLength = FMath::Clamp(Distance * CameraData->ArmLengthScale,
	                                       CameraData->MinArmLength, CameraData->MaxArmLength);
}

AActor* UCameraMode_Exploration::FindTargetActor() const
{
	if (!CameraComponent)
	{
		return nullptr;
	}

	UWorld* World = CameraComponent->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 월드의 모든 Actor 중 TargetActorTag를 가진 액터를 찾음
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->ActorHasTag(TargetActorTag))
		{
			return Actor;
		}
	}

	return nullptr;
}
