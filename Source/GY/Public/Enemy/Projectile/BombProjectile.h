#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "BombProjectile.generated.h"

class AAreaWarningActor;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class GY_API ABombProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	ABombProjectile();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void PostInitializeComponents() override;

	/** 클라(시뮬레이티드 프록시)는 로컬 PMC 시뮬 끔 — 중력 로컬 시뮬이 복제 위치와 충돌해 뚫림/떨림 발생 */
	virtual void BeginPlay() override;

	/** 플레이어 직격 → 즉시 폭발 (서버에서만 호출됨) */
	virtual void OnHitTarget(AActor* HitActor, const FHitResult& HitResult) override;

	/** 바닥 착지 → 붙어서 점멸 시작 (기본 동작인 Destroy를 막음) */
	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult) override;

private:
	/** 서버: 바닥에 붙이고 퓨즈 타이머 시작. bArmed 복제로 클라에 전파됨 */
	void Arm(const FHitResult& ImpactResult);

	/** 서버(리슨)/클라 공통: 데칼, MID, 점멸 Tick 시작 */
	void StartFuseVisuals();

	UFUNCTION()
	void OnRep_Armed();

	void Explode();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 착지 시 폭발 범위 표시용. FuseTime 동안 차오른 뒤 폭발 (비복제 액터라 서버/클라 각자 로컬 스폰) */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AAreaWarningActor> WarningAreaClass;

	UPROPERTY(Transient)
	TObjectPtr<AAreaWarningActor> WarningArea;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float ExplosionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float FuseTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float GravityScale = 1.f;

	/** 착지 시 바닥에서 띄울 높이 (메시 파묻힘 방지) */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float GroundOffset = 5.f;

	/** 점멸 시작 주파수(Hz) */
	UPROPERTY(EditDefaultsOnly, Category = "Blink")
	float StartBlinkFreq = 2.f;

	/** 폭발 직전 점멸 주파수(Hz) */
	UPROPERTY(EditDefaultsOnly, Category = "Blink")
	float EndBlinkFreq = 12.f;

	/** 머티리얼에 넘길 최대 발광 강도 */
	UPROPERTY(EditDefaultsOnly, Category = "Blink")
	float MaxGlowIntensity = 20.f;

	/** 머티리얼 스칼라 파라미터 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Blink")
	FName FlashParamName = TEXT("FlashIntensity");

	UPROPERTY(EditDefaultsOnly, Category = "Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag ExplosionCueTag;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Armed)
	bool bArmed = false;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MeshMID;

	FTimerHandle FuseTimerHandle;
	float ElapsedFuse = 0.f;
	float BlinkPhase = 0.f;

	/** 서버 전용, 복제 불필요 */
	bool bExploded = false;
};
