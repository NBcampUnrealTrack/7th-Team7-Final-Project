#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GYRegionVolume.generated.h"

class UBoxComponent;
class URegionLootData;

UCLASS()
class GY_API AGYRegionVolume : public AActor
{
	GENERATED_BODY()

public:
	AGYRegionVolume();

	// 루트박스 등이 자기 지역을 상속하는 데 사용
	const TSoftObjectPtr<URegionLootData>& GetRegionData() const { return RegionData; }
	bool IsLocationInside(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Region")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TSoftObjectPtr<URegionLootData> RegionData;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
