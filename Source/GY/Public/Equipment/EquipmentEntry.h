#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "EquipmentEntry.generated.h"

class UActiveEquipmentComponent;
class UEquipmentInstance;

USTRUCT(BlueprintType)
struct GY_API FEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FGameplayTag SlotTag;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEquipmentInstance> Instance;

	void PreReplicatedRemove(const struct FEquipmentList& Serializer);
	void PostReplicatedAdd(const struct FEquipmentList& Serializer);
	void PostReplicatedChange(const struct FEquipmentList& Serializer);
};

USTRUCT(BlueprintType)
struct GY_API FEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FEquipmentEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActiveEquipmentComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FEquipmentEntry, FEquipmentList>(
			Entries, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FEquipmentList> : public TStructOpsTypeTraitsBase2<FEquipmentList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
