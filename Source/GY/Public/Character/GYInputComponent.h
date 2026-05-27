// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GYInputConfig.h"
#include "GYInputComponent.generated.h"

//이 클래스를 템플릿 함수를 사용함. cpp 파일 맥주(비어)있음
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGYInputComponent(const FObjectInitializer& ObjectInitializer);

	//BindAction을 대신할 BindNativeAction 정의
	template <class UserClass, typename FuncType>
	void BindNativeAction(const UGYInputConfig* InputConfig, const FGameplayTag& InputTag,
	                      ETriggerEvent TriggerEvent,
	                      UserClass* Object,
	                      FuncType Func,
	                      bool bLogIfNotFound)
	{
		check(InputConfig);
		if (const UInputAction* FoundAction = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
		{
			BindAction(FoundAction, TriggerEvent, Object, Func);
		}

	}

	//어빌리티 입력 일괄 바인딩 — InputTag을 콜백 인자로 넘김
	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UGYInputConfig* InputConfig, UserClass* Object,
	                        PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
	{
		check(InputConfig);
		for (const FGYInputAction& Action : InputConfig->AbilityInputActions)
		{
			if (Action.InputAction == nullptr || !Action.InputTag.IsValid()) continue;

			if (PressedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag);
			}
			if (ReleasedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
			}
		}
	}


protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
