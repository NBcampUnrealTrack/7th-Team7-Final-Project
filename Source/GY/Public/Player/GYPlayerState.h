#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
#include "GameFramework/PlayerState.h"
#include "GYPlayerState.generated.h"

class UAbilitySystemComponent;
class UCurrencyComponent;
class UEquipmentLoadoutComponent;
class UGYAbilitySystemComponent;
class UGYPawnData;
class UInventoryComponent;
class UGYPlayerBaseAttribute;
class UGYPlayerAdditionalAttribute;
class UGYPlayerAttribute;

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
	UEquipmentLoadoutComponent* GetEquipmentLoadoutComponent() const { return EquipmentLoadoutComponent; }

	UFUNCTION(BlueprintPure)
	UCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }

protected:
	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UGYPawnData> PawnData;

	UPROPERTY()
	FAbilitySetGrantedHandles GrantedHandles;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGYAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEquipmentLoadoutComponent> EquipmentLoadoutComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCurrencyComponent> CurrencyComponent;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYPlayerBaseAttribute> BaseAttribute;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYPlayerAdditionalAttribute> AdditionalAttribute;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYPlayerAttribute> PlayerAttribute;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYWeaponAttribute> WeaponAttribute;
};
