// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "GYInputConfig.generated.h"

/**
 *
 */

class UInputAction;

USTRUCT(BlueprintType)
struct FGYInputAction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(Category = "InputTag"))
	FGameplayTag InputTag;
};

UCLASS()
class GY_API UGYInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//직접 함수 바인딩용 (이동/시야 등). 태그-액션 한 쌍씩
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FGYInputAction> NativeInputActions;

	//어빌리티 입력용 (InputTag으로 ASC가 어빌리티 활성화). 태그-액션 한 쌍씩
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FGYInputAction> AbilityInputActions;

	//태그를 입력하면 해당하는 인풋액션을 찾아주는 함수
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
};
