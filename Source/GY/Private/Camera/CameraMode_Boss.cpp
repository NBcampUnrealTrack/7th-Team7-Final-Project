#include "Camera/CameraMode_Boss.h"

#include "Camera/GYCameraComponent.h"
#include "Camera/GYCameraModeData.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

void UCameraMode_Boss::ExitMode()
{
	Super::ExitMode();
	CachedBossActor.Reset();
}

void UCameraMode_Boss::UpdateCamera(float DeltaTime, FGYCameraView& OutView)
{
	Super::UpdateCamera(DeltaTime, OutView);

	APawn* Pawn = GetPawn();
	if (!Pawn || !CameraData)
	{
		return;
	}

	// 캐시된 보스가 없으면 재탐색
	if (!CachedBossActor.IsValid())
	{
		CachedBossActor = FindBossActor();
	}

	if (!CachedBossActor.IsValid())
	{
		// 보스가 없으면 Super에서 설정한 기본 뷰 유지
		return;
	}

	const FVector PlayerLoc = Pawn->GetActorLocation();
	const FVector BossLoc = CachedBossActor->GetActorLocation();
	const float Distance = FVector::Dist(PlayerLoc, BossLoc);

	// 거리 450 이전: 중간 지점(0.5), 450~500: 0.5→0.8로 보간, 500 이후: 0.8 고정
	// 보스가 멀어질수록 플레이어 위주로 화면을 구성해 보스가 화면 밖으로 나가는 것을 방지
	const float ShiftAlpha = FMath::Clamp(
		(Distance - PivotShiftStartDistance) / (PivotShiftEndDistance - PivotShiftStartDistance),
		0.f, 1.f);
	const float PlayerWeight = FMath::Lerp(0.5f, MaxPlayerWeight, ShiftAlpha);

	OutView.PivotLocation = FMath::Lerp(BossLoc, PlayerLoc, PlayerWeight);

	// 두 캐릭터가 멀어질수록 줌 아웃 (데이터 에셋 기본값 이상으로만 확장)
	OutView.TargetArmLength = FMath::Max(
		CameraData->TargetArmLength,
		Distance * ZoomDistanceFactor);
}

AActor* UCameraMode_Boss::FindBossActor() const
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

	// 월드의 모든 Actor 중 BossActorTag를 가진 액터를 찾음
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->ActorHasTag(BossActorTag))
		{
			return Actor;
		}
	}

	return nullptr;
}