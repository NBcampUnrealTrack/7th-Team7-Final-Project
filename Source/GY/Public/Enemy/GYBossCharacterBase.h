#pragma once

#include "CoreMinimal.h"
#include "GYEnemyCharacterBase.h"
#include "Config/BossDataAsset.h"
#include "Component/BossPhaseComponent.h"
#include "GYBossCharacterBase.generated.h"

class UCurveTable;
class UBossPhaseComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEncounterStarted, int32, ParticipantCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossParticipantCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossMinionCountChanged, int32, NewCount);

USTRUCT()
struct FBossCachedSummonable
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UEnemyDataAsset> DataAsset;

	UPROPERTY()
	TSubclassOf<AGYEnemyCharacterBase> ActorClass;

	float HealthBleedRatio  = 1.f;
};

UCLASS()
class GY_API AGYBossCharacterBase : public AGYEnemyCharacterBase
{
	GENERATED_BODY()

public:
	AGYBossCharacterBase();

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
	UBossDataAsset* GetBossData() const { return Cast<UBossDataAsset>(LoadedDataAsset); }

	bool GetSummonable(EEnemyType Type, FBossCachedSummonable& Out) const;

	void RegisterMinion(AGYEnemyCharacterBase* Minion, float HealthBleedRatio);

	UFUNCTION(BlueprintPure, Category = "Boss|Minion")
	int32 GetActiveMinionCount() const { return ActiveMinionRatios.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Minion")
	FOnBossMinionCountChanged OnMinionCountChanged;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float GetStatScaleValue() const override;
	virtual void OnDataAssetLoaded() override;
	virtual void GrantDefaultAbilities() override;

	UFUNCTION()
	void OnRep_Participants();

	void HandleStaggerBegin() override;
	void HandleStunBegin() override;
	void Die() override;

	UFUNCTION()
	void OnPhaseQueued(const FBossPhaseTrigger& Trigger);

	void ApplyPhaseSetup(const FBossPhaseSetup& Setup);

	void RequestSummonablePreload();
	void OnSummonablesLoaded();

	UFUNCTION()
	void HandleMinionDamaged(AGYEnemyCharacterBase* Minion, float DamageAmount);

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
	TMap<EEnemyType, FBossCachedSummonable> SummonCache;

	UPROPERTY(Transient)
	TMap<TObjectPtr<AGYEnemyCharacterBase>, float> ActiveMinionRatios;
};
