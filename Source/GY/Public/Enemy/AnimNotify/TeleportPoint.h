#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "TeleportPoint.generated.h"

/**
 * 잡몹 텔포 어빌리티의 위치 전환 시점 트리거 노티.
 *
 * Notify 가 실행되면 Owner(Enemy) 에 Event.Enemy.Teleport.Trigger 게임플레이 이벤트를 발사.
 * UEnemyTeleport 어빌리티의 WaitGameplayEvent 가 이를 받아 SetActorLocation + AppearCue 실행.
 */
UCLASS()
class GY_API UTeleportPoint : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
