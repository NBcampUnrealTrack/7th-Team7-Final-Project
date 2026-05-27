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

	// 워커 스레드에서 락 없이 메모리 주소를 반환할 핵심 함수 (프로퍼티 액세스용)
	UFUNCTION(BlueprintPure, Category = "Animation", meta = (BlueprintThreadSafe))
	UGYCharacterAnimInstance* GetMainAnimBPThreadSafe() const;

protected:
	// 형변환(Cast) 연산 비용을 아끼기 위해 저장해두는 포인터
	UPROPERTY(Transient)
	UGYCharacterAnimInstance* MainAnimInstance;
};
