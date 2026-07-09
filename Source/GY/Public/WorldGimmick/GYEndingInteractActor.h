#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/Interactable.h"
#include "GYEndingInteractActor.generated.h"

class UBoxComponent;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

/**
 * 시네마틱 동작 - 보스전 직전, 직후
 */
UENUM(BlueprintType)
enum class EGYCinematicGateMode : uint8
{
	Intro    UMETA(DisplayName = "Intro (Before Boss)"),
	Ending   UMETA(DisplayName = "Ending (After Boss)"),
};

/**
 * 레벨 시퀀스 재생 액터
 */
UCLASS()
class GY_API AGYEndingInteractActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AGYEndingInteractActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	// 보스 사망 연출(시퀀서)이 끝난 뒤, 서버 권위로 실제 위치를 이동시킴
	void RevealAtDesignatedLocation();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> InteractionBox;


	UPROPERTY(EditAnywhere, Category = "Cinematic")
	FText InteractionText;
	UPROPERTY(EditAnywhere, Category = "Cinematic")
	FGameplayTag InteractTag;

	UPROPERTY(EditAnywhere, Category = "Cinematic", meta = (Categories = "State"))
	FGameplayTag RequiredStateTag; // 보스 처치 상태 태그 - 보스 잡기 전엔 상호작용 불가능하게
	UPROPERTY(EditAnywhere, Category = "Cinematic")
	TSoftObjectPtr<ULevelSequence> Cinematic;

	UPROPERTY(EditAnywhere, Category = "Cinematic")
	FName BossBindingTag = TEXT("Boss");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Cinematic")
	EGYCinematicGateMode Mode = EGYCinematicGateMode::Ending;

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyWaiting(APlayerState* ChangedPlayer, int32 Current, int32 Required, bool bAdded);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCinematic(const FSoftObjectPath& SequencePath);
	UFUNCTION()
	void HandleCinematicFinished();

	void BroadcastCinematicFinishedLocal();
	int32 GetCurrentPlayerCount() const;

	UPROPERTY()
	TArray<TWeakObjectPtr<APlayerState>> InteractedPlayers;

	UPROPERTY()
	int32 RequiredCount = 0;

	UPROPERTY(Replicated)
	bool bConsumed = false;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	// 보스 사망 연출 종료 후 이 액터가 실제로 이동할 위치 (서버 권위, Outro 전용)
	UPROPERTY(EditInstanceOnly, Category = "Cinematic")
	TObjectPtr<AActor> RevealTargetPoint;

	bool bRevealed = false;

	// 시네마틱 재생 후 처리 (Intro/Ending 공용)
	UPROPERTY(EditInstanceOnly, Category = "Cinematic")
	TObjectPtr<AActor> PostCinematicSpawnPoint;

	UPROPERTY(EditInstanceOnly, Category = "Cinematic")
	TObjectPtr<AActor> BlockingActor;

	UPROPERTY(EditInstanceOnly, Category = "Cinematic")
	float CinematicDuration = 5.0f;

	FTimerHandle PostCinematicTimerHandle;

	// 시네마틱 재생 직전, 플레이어 원점으로 이동
	void TeleportAllPawnsToOrigin();

	void OnPostCinematicTimerExpired();
};
