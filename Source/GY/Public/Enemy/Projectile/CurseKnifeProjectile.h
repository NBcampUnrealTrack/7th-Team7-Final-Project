#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "CurseKnifeProjectile.generated.h"

class UGameplayEffect;
class APoisonZoneActor;

// 저주 단검 발사체. 플레이어 적중 시 저주 마크 GE 부여, 빗나가면 착탄 지점에 독 장판 스폰
UCLASS()
class GY_API ACurseKnifeProjectile : public AProjectileBase
{
	GENERATED_BODY()

protected:
	// 대상 적중: 기존 대미지 이벤트/HitCue 유지 + 저주 마크 GE 적용
	virtual void OnHitTarget(AActor* HitActor, const FHitResult& HitResult) override;

	// 지면 충돌: 독 장판 스폰 후 소멸 (base의 HitCue 재생은 장판 FX로 대체하므로 생략)
	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult) override;

	/** 적중한 플레이어에게 부여할 저주 마크 GE. Duration형, 만료 시 폭발 GE가 발동되도록 구성 */
	UPROPERTY(EditDefaultsOnly, Category = "Curse")
	TSubclassOf<UGameplayEffect> CurseMarkEffectClass;

	/** 빗나갔을 때 착탄 지점에 스폰할 독 장판 */
	UPROPERTY(EditDefaultsOnly, Category = "Curse")
	TSubclassOf<APoisonZoneActor> ZoneClass;
};
