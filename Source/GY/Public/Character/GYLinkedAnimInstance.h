// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GYLinkedAnimInstance.generated.h"

class UGYCharacterAnimInstance;
/**
 *
 */
UCLASS()
class GY_API UGYLinkedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;


	UFUNCTION(BlueprintPure, Category = "Animation", meta = (BlueprintThreadSafe))
	UGYCharacterAnimInstance* GetMainAnimBPThreadSafe() const;

protected:
	
	UPROPERTY(Transient)
	UGYCharacterAnimInstance* MainAnimInstance;
};
