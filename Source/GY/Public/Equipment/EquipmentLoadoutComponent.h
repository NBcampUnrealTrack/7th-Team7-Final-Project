#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EquipmentLoadoutComponent.generated.h"

USTRUCT(BlueprintType)
struct GY_API FEquipmentLoadoutEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag SlotTag;

	UPROPERTY()
	FGuid InstanceId;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLoadoutSlotChanged, FGameplayTag /*SlotTag*/, FGuid /*NewInstanceId*/);

UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class GY_API UEquipmentLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentLoadoutComponent();

	UFUNCTION(Server, Reliable)
	void Server_RequestEquip(const FGuid& InstanceId);

	UFUNCTION(Server, Reliable)
	void Server_RequestUnequip(FGameplayTag SlotTag);

	bool SetSlot(FGameplayTag SlotTag, const FGuid& InstanceId);
	bool ClearSlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintPure)
	bool GetSlot(FGameplayTag SlotTag, FGuid& OutInstanceId) const;

	const TArray<FEquipmentLoadoutEntry>& GetEntries() const { return LoadoutEntries; }

	FOnLoadoutSlotChanged OnLoadoutSlotChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LoadoutEntries(const TArray<FEquipmentLoadoutEntry>& OldEntries);

	UPROPERTY(ReplicatedUsing = OnRep_LoadoutEntries)
	TArray<FEquipmentLoadoutEntry> LoadoutEntries;
};
