// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYInputConfig.h"
#include "Logging/GYLogManager.h"
const UInputAction* UGYInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FGYInputAction& ActionStruct : NativeInputActions)
	{
		if (ActionStruct.InputAction && (ActionStruct.InputTag == InputTag))
		{
			return ActionStruct.InputAction;
		}
	}

	if (bLogNotFound)
	{
		GY_ERROR(Player, KHB, "태그에 해당되는 액션 없음")
	}

	return nullptr;

}

const UInputAction* UGYInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FGYInputAction& ActionStruct : AbilityInputActions)
	{
		if (ActionStruct.InputAction && (ActionStruct.InputTag == InputTag))
		{
			return ActionStruct.InputAction;
		}
	}

	if (bLogNotFound)
	{
		GY_ERROR(Player, KHB, "태그에 해당되는 어빌리티 액션 없음")
	}

	return nullptr;
}
