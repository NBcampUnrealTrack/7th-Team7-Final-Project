#pragma once

#include "CoreMinimal.h"
#include "World/VolumeActor/GYTriggerVolumeBase.h"
#include "GYBossTriggerVolume.generated.h"

class AGYEnemyCharacterBase;
class APawn;

/**
 * 보스 HUD 전용 트리거 볼륨 - 멀티플레이 대응
 */
UCLASS()
class GY_API AGYBossTriggerVolume : public AGYTriggerVolumeBase
{
	GENERATED_BODY()

public:
	AGYBossTriggerVolume();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void HandlePawnEntered(APawn* Pawn) override;
	virtual void HandlePawnExited(APawn* Pawn) override;

	UPROPERTY(EditInstanceOnly, Category = "Boss")
	TSoftObjectPtr<AGYEnemyCharacterBase> TargetBoss;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float ResolveRetryInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	int32 ResolveRetryMaxCount = 40;

	UPROPERTY(EditAnywhere, Category = "Boss", meta = (ClampMin = "0.05"))
	float InsideCheckInterval = 0.5f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_ResolvedBoss)
	TObjectPtr<AGYEnemyCharacterBase> ResolvedBoss;

	UFUNCTION()
	void OnRep_ResolvedBoss();

	void ServerTryResolveBoss();
	void TryShowBossLocal();
	void ShowBoss(AGYEnemyCharacterBase* Boss);
	void HideBoss();

	bool IsLocalPlayerPawn(const APawn* Pawn) const;

	APawn* GetLocalPlayerPawn() const;

	void StartInsideCheck();
	void StopInsideCheck();
	void VerifyLocalPlayerStillInside();

	bool bLocalPlayerInside = false;
	bool bShown = false;

	int32 ServerResolveRetryCount = 0;
	int32 ShowRetryCount = 0;

	FTimerHandle ServerResolveTimer;
	FTimerHandle ShowRetryTimer;
	FTimerHandle InsideCheckTimer;

	TWeakObjectPtr<APawn> TrackedLocalPawn;
};
