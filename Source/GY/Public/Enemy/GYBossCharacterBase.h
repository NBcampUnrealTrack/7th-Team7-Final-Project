#pragma once

#include "CoreMinimal.h"
#include "GYEnemyCharacterBase.h"
#include "Config/BossDataAsset.h"
#include "GYBossCharacterBase.generated.h"

class UCurveTable;
class UBossPhaseComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEncounterStarted, int32, ParticipantCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossParticipantCountChanged, int32, NewCount);

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
protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float GetStatScaleValue() const override;

	UFUNCTION()
	void OnRep_Participants();

	void HandleStaggerBegin() override;
	void HandleStunBegin() override;
	void Die() override;
protected:
	UPROPERTY(ReplicatedUsing = OnRep_Participants, VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Encounter")
	TArray<TObjectPtr<APlayerState>> Participants;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossPhaseComponent> PhaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Encounter")
	bool bEncounterStarted = false;

};

