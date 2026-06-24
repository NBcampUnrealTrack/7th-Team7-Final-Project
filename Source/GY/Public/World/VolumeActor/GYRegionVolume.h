#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GYRegionVolume.generated.h"

class UBoxComponent;
class URegionLootData;
class APawn;

UCLASS()
class GY_API AGYRegionVolume : public AActor
{
	GENERATED_BODY()

public:
	AGYRegionVolume();

	// 루트박스 등이 자기 지역을 상속하는 데 사용
	const TSoftObjectPtr<URegionLootData>& GetRegionData() const { return RegionData; }
	bool IsLocationInside(const FVector& WorldLocation) const;

	// 네트워크 동기화 변수 등록용
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Region")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TSoftObjectPtr<URegionLootData> RegionData;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TargetBossActor, Category = "Region")
	TObjectPtr<AActor> TargetBossActor;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_TargetBossActor();

	void HandlePawnEntered(APawn* Pawn);
	void HandlePawnExited(APawn* Pawn);
	void ProcessInitialOverlappingPawns(); // 시작 시 볼륨 내부 폰 누락 방지
	void TryNotifyLocalPawn(); // 로컬 폰 동기화 지연 방어 - 재시도

	void TryBroadcastForLocalPawn();

	FTimerHandle LocalPawnRetryTimer;
	int32 LocalPawnRetryCount = 0;
};
