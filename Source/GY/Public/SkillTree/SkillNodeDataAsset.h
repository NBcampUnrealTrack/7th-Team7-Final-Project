#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Engine/DataAsset.h"
#include "SkillNodeDataAsset.generated.h"

/**
 *
 */
UCLASS()
class GY_API USkillNodeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillNode")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillNode")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillNode")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillNode")
	TArray<TObjectPtr<USkillNodeDataAsset>> Prerequisites;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillNode")
	TArray<TObjectPtr<USkillNodeDataAsset>> Children;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillNode")
	TSubclassOf<UGameplayEffect> SkillEffect;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SkillNode",GetFName());
	}
};
