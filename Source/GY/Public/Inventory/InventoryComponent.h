#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemEnums.h"
#include "Inventory/InventoryEntry.h"
#include "Templates/Function.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged,
	const FGuid& /*InstanceId*/,
	EInventoryEventType /*EventType*/);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class GY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	bool TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId);
	bool TryRemoveItem(const FGuid& InstanceId, int32 Count);
	bool MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator);
	const FInventoryEntry* FindEntry(const FGuid& InstanceId) const;
	TArray<FInventoryEntry> GetAllEntriesByCategory(FGameplayTag CategoryTag) const;

	FOnInventoryChanged OnInventoryChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	FInventoryList Inventory;
};
