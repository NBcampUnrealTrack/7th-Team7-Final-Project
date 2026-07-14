#include "Core/GameplayCue/GYGameplayCueNotify_PostProcess.h"

#include "Logging/GYLogManager.h"
#include "Camera/GYCameraComponent.h"

bool AGYGameplayCueNotify_PostProcess::OnActive_Implementation(AActor* MyTarget,
                                                               const FGameplayCueParameters& Parameters)
{
	GY_WARN(Player, CYS, "포스트프로세스 게임플레이큐 실행");
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

	CameraComp->ApplyPostProcess(PostProcessMaterial);

	return true;
}

bool AGYGameplayCueNotify_PostProcess::OnRemove_Implementation(AActor* MyTarget,
	const FGameplayCueParameters& Parameters)
{
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

	CameraComp->RemovePostProcess(PostProcessMaterial);

	return true;
}
