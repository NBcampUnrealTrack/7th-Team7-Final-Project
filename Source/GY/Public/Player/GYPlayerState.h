#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GYPlayerState.generated.h"

class UAbilitySystemComponent;
class UEquipmentLoadoutComponent;
class UGYAbilitySystemComponent;
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

	UGYAbilitySystemComponent* GetGYAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure)
	UEquipmentLoadoutComponent* GetEquipmentLoadoutComponent() const { return EquipmentLoadoutComponent; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGYAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEquipmentLoadoutComponent> EquipmentLoadoutComponent;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYPlayerBaseAttribute> BaseAttribute;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYPlayerAdditionalAttribute> AdditionalAttribute;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UGYPlayerAttribute> PlayerAttribute;
};
