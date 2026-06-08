#pragma once

#include "CoreMinimal.h"
#include "Enchant/RolledEnchantOption.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Items/ItemContainer.h"
#include "InventoryEntry.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct GY_API FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FGuid InstanceId;

	UPROPERTY(VisibleAnywhere)
	TSoftObjectPtr<UItemDefinition> Definition;

	UPROPERTY(VisibleAnywhere)
	int32 StackCount = 1;

	UPROPERTY(VisibleAnywhere)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere)
	int32 EnhancementLevel = 0;

	UPROPERTY(VisibleAnywhere)
	FGameplayTag GradeTag;

	UPROPERTY(VisibleAnywhere)
	TArray<FRolledEnchantOption> RolledOptions;

	UPROPERTY(VisibleAnywhere)
	TArray<FGuid> SocketedGemInstanceIds;

	UPROPERTY(VisibleAnywhere)
	float StatDeviation = 0.f;

	UPROPERTY(VisibleAnywhere)
	int32 RandomSeed = 0;

	void PreReplicatedRemove(const struct FInventoryList& Serializer);
	void PostReplicatedAdd(const struct FInventoryList& Serializer);
	void PostReplicatedChange(const struct FInventoryList& Serializer);
};

USTRUCT(BlueprintType)
struct GY_API FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TScriptInterface<IItemContainer> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(
			Entries, DeltaParms, *this);
	}

	// 델타 적용이 끝난 뒤 클라에서 1회 호출. PreReplicatedRemove 시점엔 배열이 아직 stale하므로
	// 최종 일관 상태로 UI를 갱신하려면 여기서 알린다.
	void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters);
};

template <>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
