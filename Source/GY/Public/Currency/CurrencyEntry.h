#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "CurrencyEntry.generated.h"

class UCurrencyComponent;

USTRUCT(BlueprintType)
struct GY_API FCurrencyEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FGameplayTag CurrencyTag;

	UPROPERTY(VisibleAnywhere)
	int32 Amount = 0;

	void PreReplicatedRemove(const struct FCurrencyList& Serializer);
	void PostReplicatedAdd(const struct FCurrencyList& Serializer);
	void PostReplicatedChange(const struct FCurrencyList& Serializer);
};

USTRUCT(BlueprintType)
struct GY_API FCurrencyList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FCurrencyEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UCurrencyComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FCurrencyEntry, FCurrencyList>(
			Entries, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FCurrencyList> : public TStructOpsTypeTraitsBase2<FCurrencyList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
