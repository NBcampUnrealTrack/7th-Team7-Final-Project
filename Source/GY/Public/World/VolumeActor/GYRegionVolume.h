#pragma once

#include "CoreMinimal.h"
#include "World/VolumeActor/GYTriggerVolumeBase.h"
#include "GYRegionVolume.generated.h"

class URegionLootData;
class APawn;

UCLASS()
class GY_API AGYRegionVolume : public AGYTriggerVolumeBase
{
	GENERATED_BODY()

public:
	// 루트박스 등이 자기 지역을 상속하는 데 사용
	const TSoftObjectPtr<URegionLootData>& GetRegionData() const { return RegionData; }

protected:
	virtual void HandlePawnEntered(APawn* Pawn) override;
	virtual void HandlePawnExited(APawn* Pawn) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TSoftObjectPtr<URegionLootData> RegionData;
};
