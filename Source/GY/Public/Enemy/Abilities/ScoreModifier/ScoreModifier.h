// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScoreModifier.generated.h"

class UAbilitySystemComponent;
/**
 *
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class GY_API UScoreModifier : public UObject
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const UAbilitySystemComponent* ASC,
						   AActor* Owner, AActor* Target) const
	{
		return 0.f;
	}
};
