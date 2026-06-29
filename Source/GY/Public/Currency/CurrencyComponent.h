#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Currency/CurrencyEntry.h"
#include "Persistence/GYSaveable.h"
#include "CurrencyComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCurrencyChanged,
	FGameplayTag /*CurrencyTag*/,
	int32 /*NewAmount*/);

UCLASS(ClassGroup = (Currency), meta = (BlueprintSpawnableComponent))
class GY_API UCurrencyComponent : public UActorComponent, public IGYSaveable
{
	GENERATED_BODY()

public:
	UCurrencyComponent();

	UFUNCTION(BlueprintPure)
	int32 GetAmount(FGameplayTag CurrencyTag) const;

	bool TryAdd(FGameplayTag CurrencyTag, int32 Amount);
	bool TrySpend(FGameplayTag CurrencyTag, int32 Amount);

	const TArray<FCurrencyEntry>& GetEntries() const { return Currencies.Entries; }

	FOnCurrencyChanged OnCurrencyChanged;

	// IGYSaveable
	virtual FString GetSaveSectionKey() const override { return TEXT("currency"); }
	virtual TSharedPtr<FJsonValue> ExportSaveData() const override;
	virtual void ImportSaveData(const TSharedPtr<FJsonValue>& Data) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Currency")
	FCurrencyList Currencies;
};
