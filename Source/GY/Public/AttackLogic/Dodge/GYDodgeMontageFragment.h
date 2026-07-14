#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GYDodgeMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct FGYDirectionalMontage
{
	GENERATED_BODY()

	// 0=전방, 90=우, -90=좌, 180=후방
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="-180", ClampMax="180", Units="deg"))
	float Angle = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage;
};

UENUM(BlueprintType)
enum class EGYDodgeMontageMode : uint8
{
	EightDirection UMETA(DisplayName = "8 Direction"),
	FrontOnly      UMETA(DisplayName = "Front Only"),
};

USTRUCT(BlueprintType)
struct GY_API FGYDodgeMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGYDodgeMontageMode Mode = EGYDodgeMontageMode::EightDirection;

	//방향에 따른 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "Mode == EGYDodgeMontageMode::EightDirection", EditConditionHides))
	TArray<FGYDirectionalMontage> DirectionalMontages;
	UAnimMontage* GetMontageByAngle(float Angle) const;
	//

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "Mode == EGYDodgeMontageMode::FrontOnly", EditConditionHides))
	TObjectPtr<UAnimMontage> DodgeMontage;

	UAnimMontage* GetSelectedMontage(float Angle) const;

	bool HasValidMontage() const;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDodgeMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDodgeMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TMap<FGameplayTag, FGYDodgeMontageSet> MontageAnimSets;



	const FGYDodgeMontageSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
