#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryEntry.generated.h"

class UInventoryComponent;
class UItemDefinition;

USTRUCT(BlueprintType)
struct GY_API FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid InstanceId;

	UPROPERTY()
	TSoftObjectPtr<UItemDefinition> Definition;

	UPROPERTY()
	int32 StackCount = 1;

	UPROPERTY()
	int32 EnhancementLevel = 0;

	UPROPERTY()
	FGameplayTag GradeTag;

	UPROPERTY()
	TArray<FName> OptionIds;

	UPROPERTY()
	TArray<FGuid> SocketedGemInstanceIds;

	UPROPERTY()
	float StatDeviation = 0.f;

	UPROPERTY()
	int32 RandomSeed = 0;

	void PreReplicatedRemove(const struct FInventoryList& Serializer);
	void PostReplicatedAdd(const struct FInventoryList& Serializer);
	void PostReplicatedChange(const struct FInventoryList& Serializer);
};

USTRUCT(BlueprintType)
struct GY_API FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UInventoryComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(
			Entries, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
