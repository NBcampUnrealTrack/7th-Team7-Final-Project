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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Cinematic")
	EGYCinematicGateMode Mode = EGYCinematicGateMode::Ending;

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyWaiting(APlayerState* ChangedPlayer, int32 Current, int32 Required, bool bAdded);
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


};
