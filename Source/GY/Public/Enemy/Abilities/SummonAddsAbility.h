#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "SummonAddsAbility.generated.h"


class AGYEnemyCharacterBase;

USTRUCT(BlueprintType)
struct FSummonEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EEnemyType EnemyType = EEnemyType::None;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float Weight = 1.f;
};

UCLASS(Abstract, Blueprintable)
class GY_API USummonAddsAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()

public:
	USummonAddsAbility();
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|Summon")
	void ExecuteSummon();

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|Summon")
	void ExecuteSummonAt(FVector CenterLocation);
private:
	const FSummonEntry* PickRandomEntry() const;

	AGYEnemyCharacterBase* SpawnAndInitMinion(
		const FSummonEntry& Entry,
		const FVector& Location,
		const FRotator& Rotation) const;
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|Summon")
	TArray<FSummonEntry> SummonPool;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|Summon")
	int32 MinionCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|Summon")
	bool bSpawnAroundTarget = false;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|Summon")
	float SpawnRadiusMin = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|Summon")
	float SpawnRadiusMax = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|Summon")
	FGameplayTag SummonCueTag;


};
