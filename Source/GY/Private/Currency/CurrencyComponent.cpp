#include "Currency/CurrencyComponent.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GameplayTagsManager.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

void FCurrencyEntry::PreReplicatedRemove(const FCurrencyList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnCurrencyChanged.Broadcast(CurrencyTag, 0);
	}
}

void FCurrencyEntry::PostReplicatedAdd(const FCurrencyList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnCurrencyChanged.Broadcast(CurrencyTag, Amount);
	}
}

void FCurrencyEntry::PostReplicatedChange(const FCurrencyList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnCurrencyChanged.Broadcast(CurrencyTag, Amount);
	}
}

UCurrencyComponent::UCurrencyComponent()
{
	SetIsReplicatedByDefault(true);
	Currencies.OwnerComponent = this;
}

void UCurrencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UCurrencyComponent, Currencies, Params);
}

int32 UCurrencyComponent::GetAmount(FGameplayTag CurrencyTag) const
{
	const FCurrencyEntry* Entry = Currencies.Entries.FindByPredicate([&CurrencyTag](const FCurrencyEntry& E)
	{
		return E.CurrencyTag == CurrencyTag;
	});
	return Entry != nullptr ? Entry->Amount : 0;
}

bool UCurrencyComponent::TryAdd(FGameplayTag CurrencyTag, int32 Amount)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!CurrencyTag.IsValid()) return false;
	if (Amount <= 0) return false;

	FCurrencyEntry* Entry = Currencies.Entries.FindByPredicate([&CurrencyTag](const FCurrencyEntry& E)
	{
		return E.CurrencyTag == CurrencyTag;
	});

	if (Entry != nullptr)
	{
		Entry->Amount += Amount;
		Currencies.MarkItemDirty(*Entry);
	}
	else
	{
		FCurrencyEntry NewEntry;
		NewEntry.CurrencyTag = CurrencyTag;
		NewEntry.Amount = Amount;
		FCurrencyEntry& Added = Currencies.Entries.Add_GetRef(NewEntry);
		Currencies.MarkItemDirty(Added);
		Entry = &Added;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UCurrencyComponent, Currencies, this);
	OnCurrencyChanged.Broadcast(CurrencyTag, Entry->Amount);
	return true;
}

bool UCurrencyComponent::TrySpend(FGameplayTag CurrencyTag, int32 Amount)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!CurrencyTag.IsValid()) return false;
	if (Amount <= 0) return false;

	FCurrencyEntry* Entry = Currencies.Entries.FindByPredicate([&CurrencyTag](const FCurrencyEntry& E)
	{
		return E.CurrencyTag == CurrencyTag;
	});

	if (Entry == nullptr) return false;
	if (Entry->Amount < Amount) return false;

	Entry->Amount -= Amount;
	Currencies.MarkItemDirty(*Entry);
	MARK_PROPERTY_DIRTY_FROM_NAME(UCurrencyComponent, Currencies, this);
	OnCurrencyChanged.Broadcast(CurrencyTag, Entry->Amount);
	return true;
}

TSharedPtr<FJsonValue> UCurrencyComponent::ExportSaveData() const
{
	// { "Currency.TimeShard": 100, ... } — 태그 문자열 → 수량
	const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
	for (const FCurrencyEntry& Entry : Currencies.Entries)
	{
		if (Entry.CurrencyTag.IsValid())
		{
			Object->SetNumberField(Entry.CurrencyTag.ToString(), Entry.Amount);
		}
	}
	return MakeShared<FJsonValueObject>(Object);
}

void UCurrencyComponent::ImportSaveData(const TSharedPtr<FJsonValue>& Data)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Data->TryGetObject(Object) || Object == nullptr) return;

	// 기존 항목 비우고 세이브 값으로 재구축
	Currencies.Entries.Reset();

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Object)->Values)
	{
		const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Pair.Key), false);
		if (!Tag.IsValid()) continue;

		FCurrencyEntry NewEntry;
		NewEntry.CurrencyTag = Tag;
		NewEntry.Amount = static_cast<int32>(Pair.Value->AsNumber());
		FCurrencyEntry& Added = Currencies.Entries.Add_GetRef(NewEntry);
		Currencies.MarkItemDirty(Added);

		OnCurrencyChanged.Broadcast(Tag, Added.Amount);
	}

	Currencies.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UCurrencyComponent, Currencies, this);
}
