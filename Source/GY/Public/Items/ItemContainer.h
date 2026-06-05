#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/Types/ItemEnums.h"

#include "ItemContainer.generated.h"

class UItemDefinition;
struct FInventoryEntry;

UINTERFACE(MinimalAPI, Blueprintable)
class UItemContainer : public UInterface
{
	GENERATED_BODY()
};

class GY_API IItemContainer
{
	GENERATED_BODY()

public:
	// 반환: 실제로 추가된 수량. 용량이 부족하면 가능한 만큼만 넣고 나머지는 더하지 않음 (0 = 전혀 못 넣음)
	virtual int32 TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId) =0;
	virtual bool TryRemoveItem(const FGuid& InstanceId, int32 Count) =0;
	virtual bool MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator) =0;
	virtual const FInventoryEntry* FindEntry(const FGuid& InstanceId) const =0;
	virtual const TArray<FInventoryEntry>& GetEntries() const =0;
	virtual void NotifyContainerChanged(const FGuid& InstanceId, EInventoryEventType EventType)=0;
	virtual int32 GetCapacity() const =0;

	virtual bool InsertEntry(const FInventoryEntry& Entry) = 0;
	virtual bool TakeEntry(const FGuid& InstanceId, FInventoryEntry& OutEntry) = 0;
};
