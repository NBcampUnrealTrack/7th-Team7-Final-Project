#pragma once

#include "CoreMinimal.h"
#include "Enemy/Projectile/ProjectileBase.h"
#include "SlashProjectile.generated.h"

class UBoxComponent;

// 검기 전용 발사체. 진행 방향(Yaw)만 맞추고 이펙트 모양은 에셋 그대로 유지
UCLASS()
class GY_API ASlashProjectile : public AProjectileBase
{
	GENERATED_BODY()
public:
	ASlashProjectile();

	virtual void Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed) override;

protected:
	// 검기 판정 박스. X=진행방향 두께, Y=가로 폭, Z=세로 높이 — BP에서 이펙트 실루엣에 맞춰 조정
	UPROPERTY(VisibleAnywhere, Category = "Slash")
	TObjectPtr<UBoxComponent> HitBox;
};
