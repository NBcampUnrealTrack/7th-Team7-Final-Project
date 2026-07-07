#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "ItemDefinition.generated.h"

class UItemFragment;
class UTexture2D;

UCLASS(BlueprintType, Const)
class GY_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable)
	FName ItemId; // 영구 식별자. 절대 변경 금지

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Item.Category"))
	FGameplayTagContainer CategoryTags;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
	TArray<TObjectPtr<UItemFragment>> Fragments;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("Item", ItemId);
	}

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "FragmentClass"))
	const UItemFragment* FindFragmentByClass(TSubclassOf<UItemFragment> FragmentClass) const;

	template <typename TFragment>
	const TFragment* FindFragment() const
	{
		return Cast<TFragment>(FindFragmentByClass(TFragment::StaticClass()));
	}
};
