#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
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

	// [SERVER] base 어트리뷰트 값 + 파생 스탯 초기화. 값 출처(테이블/행)는 PawnData에서 읽는다.
	// ASC ActorInfo 바인딩/faction 태그는 PawnExtension 담당.
	void InitializeBaseAttributes();

	FORCEINLINE FGuid GetLastCheckpointId() const { return LastCheckpointId; }
	void SetLastCheckpointId(const FGuid& Id);

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

	//TODO: 저장해야함
	UPROPERTY(Replicated)
	FGuid LastCheckpointId;
};
