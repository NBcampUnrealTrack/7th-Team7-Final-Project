#include "Camera/CameraMode_Combat.h"

#include "Camera/GYCameraComponent.h"
#include "Camera/GYCameraModeData.h"
#include "Character/GYCharacter.h"
#include "Logging/GYLogManager.h"

void UCameraMode_Combat::ExitMode()
{
	Super::ExitMode();
}

void UCameraMode_Combat::UpdateCamera(float DeltaTime, FGYCameraView& OutView)
{
	Super::UpdateCamera(DeltaTime, OutView);
	APawn* Pawn = GetPawn();
	if (!Pawn || !CameraData)
	{
		return;
	}

	AGYCharacter* Character = Cast<AGYCharacter>(Pawn);
	if (!Character) return;

	TWeakObjectPtr<AActor> TargetActor = Character->Target;
	if (!TargetActor.IsValid())
	{
		// 타겟이 없으면 Super에서 설정한 기본 뷰 유지
		GY_WARN(Player, CYS, "전투 모드: 타겟을 찾을 수 없음");
		return;
	}

	const FVector PlayerLoc = Pawn->GetActorLocation();
	const FVector TargetLoc = TargetActor->GetActorLocation();
	const float Distance = FVector::Dist(PlayerLoc, TargetLoc);

	// 거리 PivotShiftStartDistance 이전: PlayerWeight, PivotShiftEndDistance 이후: MaxPlayerWeight 고정, 사이는 보간
	// 타겟이 멀어질수록 플레이어 위주로 화면을 구성해 플레이어가 화면 밖으로 나가는 것을 방지
	const float ShiftAlpha = FMath::Clamp(
		(Distance - CameraData->PivotShiftStartDistance) / (CameraData->PivotShiftEndDistance - CameraData->
			PivotShiftStartDistance),
		0.f, 1.f);
	const float PlayerWeight = FMath::Lerp(CameraData->PlayerWeight, CameraData->MaxPlayerWeight, ShiftAlpha);

	OutView.PivotLocation = FMath::Lerp(TargetLoc, PlayerLoc, PlayerWeight);

	// 두 캐릭터가 멀어질수록 줌 아웃 (데이터 에셋 기본값 이상으로만 확장)
	OutView.TargetArmLength = FMath::Max(
		CameraData->TargetArmLength,
		Distance * CameraData->ArmLengthScale);
}
