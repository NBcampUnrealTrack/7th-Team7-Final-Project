// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"
#include "GYAnimTypes.generated.h"

USTRUCT(BlueprintType)
struct FGYDirectionalStopAnims
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* Forward = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* ForwardLeft = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* ForwardRight = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* Left = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* Right = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* BackwardLeft = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* BackwardRight = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* Backward = nullptr;

	// 방향에 맞는 시퀀스 포인터 반환
	UAnimSequence* GetSequenceByDirection(float InDirection) const
	{
		if (InDirection >= -22.5f && InDirection < 22.5f) return Forward;
		if (InDirection >= 22.5f && InDirection < 67.5f) return ForwardRight;
		if (InDirection >= 67.5f && InDirection < 112.5f) return Right;
		if (InDirection >= 112.5f && InDirection < 157.5f) return BackwardRight;
		if (InDirection >= -67.5f && InDirection < -22.5f) return ForwardLeft;
		if (InDirection >= -112.5f && InDirection < -67.5f) return Left;
		if (InDirection >= -157.5f && InDirection < -112.5f) return BackwardLeft;
		return Backward;
	}
};
