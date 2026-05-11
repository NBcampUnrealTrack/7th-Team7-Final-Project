#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentEntry.h"
#include "GameplayTagContainer.h"
#include "EquipmentComponent.generated.h"

class UEquipmentInstance;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, FGameplayTag /*SlotTag*/, UEquipmentInstance* /*NewInstance*/);

UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class GY_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	FOnEquipmentChanged OnEquipmentChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	FEquipmentList EquippedItems;
};
