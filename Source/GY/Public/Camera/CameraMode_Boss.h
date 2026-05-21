#pragma once

#include "CoreMinimal.h"
#include "GYCameraModeBase.h"
#include "CameraMode_Boss.generated.h"

UCLASS()
class GY_API UCameraMode_Boss : public UGYCameraModeBase
{
	GENERATED_BODY()

public:
	virtual void ExitMode() override;
	virtual void UpdateCamera(float DeltaTime, FGYCameraView& OutView) override;

private:
	AActor* FindBossActor() const;

	TWeakObjectPtr<AActor> CachedBossActor;

	// TODO:: 수치 데이터 에셋으로 빼기
	// 보스 액터에 부여된 일반 태그 (에디터 Actor Tags에서 설정)
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	FName BossActorTag = FName("Boss");

	// 플레이어-보스 거리 1유닛당 암 길이 증가 비율
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	float ZoomDistanceFactor = 0.5f;

	// 이 거리부터 Pivot이 플레이어 쪽으로 이동하기 시작
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	float PivotShiftStartDistance = 300.f;

	// 이 거리에서 MaxPlayerWeight에 완전히 도달
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	float PivotShiftEndDistance = 500.f;

	// Pivot의 최대 플레이어 방향 가중치 (0.5 = 중간, 1.0 = 완전히 플레이어)
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	float MaxPlayerWeight = 0.8f;
};
