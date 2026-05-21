#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "GYComboAnimDataAsset.generated.h"

USTRUCT(BlueprintType)
struct GY_API FComboHitData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName TraceSocket = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float SphereRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bShowDebug = false;
};

USTRUCT(BlueprintType)
struct GY_API FComboAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FComboHitData> Hits;
};

UCLASS(BlueprintType)
class GY_API UGYComboAnimDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FComboAnimSet> ComboAnimSets;

	const FComboAnimSet* GetBestMatchingAnimSet(const FGameplayTagContainer& OwnedTags, const FGameplayTag& FallbackTag = FGameplayTag()) const;
};
