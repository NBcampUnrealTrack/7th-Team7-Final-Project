#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "CurveSpeedProjectile.generated.h"

UCLASS()
class GY_API ACurveSpeedProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	ACurveSpeedProjectile();

	virtual void Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed) override;
	virtual void Tick(float DeltaTime) override;

protected:
	/** X: 진행 시간, Y: 속도 배율. Launch 속도에 곱해진다 (예: 0.2 → 느림, 1.5 → 빠름) */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Speed")
	TObjectPtr<UCurveFloat> SpeedCurve;

	/** 0보다 크면 커브 X축을 0~1 정규화 구간으로 샘플링. 0이면 커브 X를 절대 초 단위로 사용 */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Speed", meta = (ClampMin = "0"))
	float CurveDuration = 0.f;

private:
	float BaseSpeed = 0.f;
	float ElapsedTime = 0.f;
};
