#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
#include "GameFramework/PlayerState.h"
#include "GYPlayerState.generated.h"

class UAltarStorageComponent;
class USkillTreeComponent;
class UAbilitySet;
class UAbilitySystemComponent;
class UCurrencyComponent;
class UEquipmentLoadoutComponent;
class UGYAbilitySystemComponent;
class UGYPawnData;
class UGYPlayerInitData;
class UInventoryComponent;
class UItemTransactionComponent;
class ULootViewerComponent;
class UGYPlayerVitalAttributeSet;
class UGYPlayerDamageAttributeSet;
class UGYCoreStatAttributeSet;
class UGYProgressionAttributeSet;

UCLASS()
class GY_API AGYPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGYPlayerState();



	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UGYAbilitySystemComponent* GetGYAbilitySystemComponent() const { return AbilitySystemComponent; }

	const UGYPawnData* GetPawnData() const { return PawnData; }
	void SetPawnData(const UGYPawnData* InPawnData);

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure)
	UAltarStorageComponent* GetAltarStorageComponent() const { return AltarStorageComponent; }

	UFUNCTION(BlueprintPure)
	UEquipmentLoadoutComponent* GetEquipmentLoadoutComponent() const { return EquipmentLoadoutComponent; }

	UFUNCTION(BlueprintPure)
	UCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }

	UFUNCTION(BlueprintPure)
	USkillTreeComponent* GetSkillTreeComponent() const { return SkillTreeComponent; }

	UFUNCTION(BlueprintPure)
	ULootViewerComponent* GetLootViewerComponent() const { return LootViewerComponent; }

	UFUNCTION(BlueprintPure)
	UItemTransactionComponent* GetItemTransactionComponent() const { return ItemTransactionComponent; }

	void InitGAS(APawn* Avatar);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Checkpoint")
	void Server_SetCheckpoint(FVector Location);

	FVector GetCheckpointLocation() const { return LastCheckpointLocation; }
	bool HasCheckpoint() const { return bHasCheckpoint; }
	FVector GetInitialSpawnLocation() const { return InitialSpawnLocation; }

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Combat")
	TObjectPtr<UGYPlayerInitData> InitData;

protected:
	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UGYPawnData> PawnData;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGYAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAltarStorageComponent> AltarStorageComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEquipmentLoadoutComponent> EquipmentLoadoutComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCurrencyComponent> CurrencyComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULootViewerComponent> LootViewerComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UItemTransactionComponent> ItemTransactionComponent;

	UPROPERTY(VisibleAnywhere, Category="SkillTree")
	TObjectPtr<USkillTreeComponent> SkillTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYPlayerVitalAttributeSet> VitalAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYPlayerDamageAttributeSet> DamageAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYCoreStatAttributeSet> CoreStatAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYProgressionAttributeSet> ProgressionAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYWeaponAttribute> WeaponAttribute;

	UPROPERTY(Replicated)
	FVector LastCheckpointLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	bool bHasCheckpoint = false;

	bool bAttributesInitialized = false;
	FVector InitialSpawnLocation = FVector::ZeroVector;
};
