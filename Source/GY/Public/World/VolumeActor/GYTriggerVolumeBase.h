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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandlePawnEntered(APawn* Pawn) {}
	virtual void HandlePawnExited(APawn* Pawn) {}

	bool IsPawnOverlapping(const APawn* Pawn) const;

	// 로컬 폰의 볼륨 포함 여부를 주기적으로 재검사
	UPROPERTY(EditAnywhere, Category = "Trigger", meta = (ClampMin = "0.1"))
	float LocalMembershipCheckInterval = 0.5f;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(Transient)
	TObjectPtr<APawn> ProcessedLocalPawn = nullptr;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ProcessInitialOverlappingPawns();
	void UpdateLocalPawnMembership();

	APawn* GetLocalPlayerPawn() const;

	bool TryMarkEntered(APawn* Pawn);

	TSet<TWeakObjectPtr<APawn>> EnteredPawns;

	FTimerHandle InitialOverlapTimer;
	FTimerHandle LocalMembershipTimer;
};
