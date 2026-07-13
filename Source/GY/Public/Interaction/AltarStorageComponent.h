#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemEnums.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemContainer.h"
#include "AltarStorageComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAltarChanged,
	const FGuid& /*InstanceId*/,
	EInventoryEventType /*EventType*/);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UAltarStorageComponent : public UActorComponent, public IItemContainer
{
	GENERATED_BODY()


public:
	UAltarStorageComponent();
	virtual void BeginPlay() override;

	virtual int32 GetCapacity() const override;
	virtual int32 TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId) override;
	virtual bool TryRemoveItem(const FGuid& InstanceId, int32 Count) override;
	virtual bool MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator) override;
	virtual const FInventoryEntry* FindEntry(const FGuid& InstanceId) const override;
	virtual const TArray<FInventoryEntry>& GetEntries() const override { return Storage.Entries; }
	virtual void NotifyContainerChanged(const FGuid& InstanceId, EInventoryEventType EventType) override;
	virtual bool InsertEntry(const FInventoryEntry& Entry) override;
	virtual bool TakeEntry(const FGuid& InstanceId, FInventoryEntry& OutEntry) override;
	virtual FGameplayTag GetContainerTag() const override;

	UFUNCTION(Server, Reliable)
	void Server_RequestDisassemble();

	// [SERVER→OWNER CLIENT] 분해 성공 시에만 사운드 재생
	UFUNCTION(Client, Reliable)
	void Client_PlayDisassembleSound(FGameplayTag SoundTag);

	FOnAltarChanged OnAltarChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Altar")
	FInventoryList Storage;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Altar")
	int32 Capacity;


};
