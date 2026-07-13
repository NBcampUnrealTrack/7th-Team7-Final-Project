#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GYTriggerVolumeBase.generated.h"

class UBoxComponent;
class APawn;

/**
 * 트리거 볼륨 공통 베이스
 */
UCLASS(Abstract)
class GY_API AGYTriggerVolumeBase : public AActor
{
	GENERATED_BODY()

public:
	AGYTriggerVolumeBase();

	bool IsLocationInside(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;
	virtual void HandlePawnEntered(APawn* Pawn) {}
	virtual void HandlePawnExited(APawn* Pawn) {}

	bool IsPawnOverlapping(const APawn* Pawn) const;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	TObjectPtr<UBoxComponent> TriggerBox;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ProcessInitialOverlappingPawns();
	void TryNotifyLocalPawn();

	FTimerHandle LocalPawnRetryTimer;
	int32 LocalPawnRetryCount = 0;
};
