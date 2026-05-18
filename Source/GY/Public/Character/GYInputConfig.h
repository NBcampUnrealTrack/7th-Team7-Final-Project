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
	//구조체를 배열로 받아 에디터에서 한쌍(태그와 입력액션)을 추가 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FGYInputAction> NativeInputActions;

	//태그를 입력하면 해당하는 인풋액션을 찾아주는 함수
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
};
