#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "AreaDenialAbility.generated.h"

class AAreaImpactProjectile;

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
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	TSubclassOf<AActor> HazardActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	int32 HazardCount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	EHazardPlacementMode PlacementMode = EHazardPlacementMode::RandomInArea;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	float PlacementRadius = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	float ArenaRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial")
	float MinSpacing = 250.f;

	/** 설정되어 있으면 Hazard 스폰 후 WarningDuration 뒤 각 Hazard 위치에 수직 낙하 projectile 발사 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	TSubclassOf<AAreaImpactProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	float WarningDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	float SpawnHeight = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|AreaDenial|Projectile")
	float DropSpeed = 3000.f;
private:
	bool IsSpacingOK(const FVector& Candidate, const TArray<FVector>& Placed) const;

	void SpawnImpactProjectiles();

	UFUNCTION()
	void OnProjectileHit(FGameplayEventData Payload);

	UPROPERTY()
	TArray<FVector> CachedImpactLocations;
};
