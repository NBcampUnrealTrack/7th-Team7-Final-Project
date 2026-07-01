#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "AreaDenialAbility.generated.h"

class AAreaImpactProjectile;
class APoisonZoneActor;

UENUM(BlueprintType)
enum class EHazardPlacementMode : uint8
{
	RandomInArea	UMETA(DisplayName = "Random In Area"),
	AroundPlayers	UMETA(DisplayName = "Around Players"),
	AroundBoss		UMETA(DisplayName = "Around Boss"),
};

UCLASS()
class GY_API UAreaDenialAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
public:
	UAreaDenialAbility();
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|AreaDenial")
	void ExecuteAreaDenail();

protected:
	/**
	 * 지면에 미리 깔리는 "경고 표시" 액터 클래스.
	 * Decal Component 또는 평면 Mesh + Material 을 가진 액터를 지정한다.
	 * WarningDuration 동안 표시된 뒤 자동 소멸한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	TSubclassOf<AActor> HazardActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|PoisonZone",
	meta = (ToolTip = "설정되면 ProjectileClass 대신 데칼 자리에 PoisonZoneActor를 스폰. Notify로 트리거됨."))
	TSubclassOf<APoisonZoneActor> PoisonZoneClass;

	/** 한 번의 Ability 발동으로 배치할 Hazard 개수. 같은 수만큼 Projectile 이 떨어진다. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	int32 HazardCount = 5;

	/**
	 * AroundBoss / AroundPlayers 모드에서 사용하는 배치 반지름.
	 * RandomInArea 모드에서는 무시된다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	EHazardPlacementMode PlacementMode = EHazardPlacementMode::RandomInArea;

	/**
	 * RandomInArea 모드에서 사용하는 아레나 반지름 (보스 중심).
	 * 다른 모드에서는 무시된다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	float PlacementRadius = 800.f;

	/**
	 * Hazard 간 최소 간격.
	 * 후보 위치가 이미 배치된 Hazard 와 이 거리보다 가까우면 재시도한다.
	 * 너무 크게 잡으면 HazardCount 를 다 못 채울 수 있다 (최대 HazardCount * 10 회 시도).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	float ArenaRadius = 2000.f;

	/**
	 * 설정되어 있으면 Hazard 스폰 후 WarningDuration 뒤 각 Hazard 위치에 수직 낙하 Projectile 발사.
	 * 비워두면 시각 경고만 표시되고 데미지는 발생하지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	float MinSpacing = 250.f;

	/** 설정되어 있으면 Hazard 스폰 후 WarningDuration 뒤 각 Hazard 위치에 수직 낙하 projectile 발사 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	TSubclassOf<AAreaImpactProjectile> ProjectileClass;

	/**
	 * Hazard 가 표시된 후 Projectile 이 스폰되기까지의 대기 시간 (초).
	 * 플레이어의 회피 윈도우. Hazard 액터의 LifeSpan 도 동일하게 설정된다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	float WarningDuration = 1.0f;

	/**
	 * Projectile 이 스폰되는 높이 (Hazard 지면 위치 기준 +Z).
	 * 이 위치에서 아래로 떨어진다. 클수록 떨어지는 시간이 길어진다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	float SpawnHeight = 2000.f;

	/** Projectile 의 낙하 속도 (cm/s). ProjectileMovement 의 InitialSpeed 로 사용된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	float DropSpeed = 3000.f;

	/**
	 * 지면 LineTrace 시작점 — 후보 위치에서 위로 얼마나 올라가서 트레이스를 시작할지.
	 * 천장이 있는 맵에서 너무 크게 잡으면 천장에 막힐 수 있다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Ground")
	float GroundTraceHeightAbove = 1000.f;

	/**
	 * 지면 LineTrace 끝점 — 후보 위치에서 아래로 얼마나 내려가며 지면을 찾을지.
	 * 절벽/낭떠러지 후보를 걸러내려면 너무 크게 잡지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Ground")
	float GroundTraceHeightBelow = 2000.f;

	/**
	 * Hazard 액터를 지면에서 띄울 거리 (cm).
	 * Z-fighting 방지용 미세 오프셋. 평면 Mesh 라면 5~10 정도가 적당하다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Ground")
	float HazardGroundOffset = 5.f;

	/**
	 * 지면 LineTrace 에 사용할 콜리전 채널.
	 * 보통 WorldStatic (지형/스태틱 메시) 이 적절. 캐릭터/플레이어 위에는 트레이스되지 않도록 주의.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_WorldStatic;

private:
	bool IsSpacingOK(const FVector& Candidate, const TArray<FVector>& Placed) const;

	void SpawnImpactProjectiles();

	UFUNCTION()
	void OnProjectileHit(FGameplayEventData Payload);

	void SpawnPoisonZones();

	UFUNCTION()
	void OnSpawnZonesEvent(FGameplayEventData Payload);
private:
	UPROPERTY()
	TArray<FVector> CachedImpactLocations;
};
