#pragma once

#include "CoreMinimal.h"
#include "GYEnemyCharacterBase.h"
#include "Config/BossDataAsset.h"
#include "Component/BossPhaseComponent.h"
#include "GYBossCharacterBase.generated.h"

class ALevelSequenceActor;
class ULevelSequencePlayer;
class ULevelSequence;
class UCurveTable;
class UBossPhaseComponent;
struct FBossCachedSummonable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEncounterStarted, int32, ParticipantCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossParticipantCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossMinionCountChanged, int32, NewCount);

UCLASS()
class GY_API AGYBossCharacterBase : public AGYEnemyCharacterBase
{
	GENERATED_BODY()

public:
	AGYBossCharacterBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Boss|Encounter")
	void SetParticipants(const TArray<APlayerState*>& InParticipants);

	UFUNCTION(BlueprintPure, Category = "Boss|Encounter")
	TArray<APlayerState*> GetParticipants() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Encounter")
	TArray<APawn*> GetParticipantPawns() const;

	UFUNCTION(BlueprintPure)
	int32 GetParticipantCount() const { return Participants.Num(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Encounter")
	bool HasEncounterStarted() const { return bEncounterStarted; }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Encounter")
	FOnBossEncounterStarted OnEncounterStarted;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Encounter")
	FOnBossParticipantCountChanged OnParticipantCountChanged;

	UFUNCTION(BlueprintPure, Category = "Boss|Data")
	UBossDataAsset* GetBossData() const;

	bool GetSummonable(EEnemyType Type, FBossCachedSummonable& Out) const;

	void RegisterMinion(AGYEnemyCharacterBase* Minion);

	UFUNCTION(BlueprintPure, Category = "Boss|Minion")
	int32 GetActiveMinionCount() const { return ActiveMinions.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Minion")
	FOnBossMinionCountChanged OnMinionCountChanged;

	UFUNCTION(BlueprintPure, Category = "Boss|Movement")
	bool IsStationary() const { return bIsStationary; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossPhaseComponent* GetPhaseComponent() const { return PhaseComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float GetStatScaleValue() const override;

	UFUNCTION()
	void OnRep_Participants();

	virtual void HandleStaggerBegin() override;
	virtual void HandleStunBegin() override;
	virtual void Die() override;

	UFUNCTION()
	void HandleMinionDead(AGYEnemyCharacterBase* Minion);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Participants, VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Encounter")
	TArray<TObjectPtr<APlayerState>> Participants;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossPhaseComponent> PhaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Encounter")
	bool bEncounterStarted = false;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AGYEnemyCharacterBase>> ActiveMinions;

	FTimerHandle TempEncounterTimer;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Movement")
	bool bIsStationary = false;

private:
	// 보스 시네마틱
	UPROPERTY(EditAnywhere, Category = "Cinematic")
	TSoftObjectPtr<ULevelSequence> Cinematic;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCinematic(const FSoftObjectPath& SequencePath);
	UFUNCTION()
	void HandleCinematicFinished();
};
