#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GYDirectionFragment.generated.h"

UENUM(BlueprintType)
enum class EGYDirectionMode : uint8
{
	ByMouseDirection    UMETA(DisplayName = "By Mouse Direction"),
	ByMovementDirection UMETA(DisplayName = "By Movement Direction"),
	ByCharacterForward  UMETA(DisplayName = "By Character Forward"),
	ByLockOnTarget      UMETA(DisplayName = "By Lock-On Target"),
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDirectionFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDirectionFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Direction")
	EGYDirectionMode DirectionMode = EGYDirectionMode::ByCharacterForward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Direction", meta = (ClampMin = "0.0", Units = "s"))
	float LerpTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Direction")
	bool bCanOverrideLockOn = false;
};
