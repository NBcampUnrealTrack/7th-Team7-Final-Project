#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "AreaDenialAbility.generated.h"


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
private:
	bool IsSpacingOK(const FVector& Candidate, const TArray<FVector>& Placed) const;
};
