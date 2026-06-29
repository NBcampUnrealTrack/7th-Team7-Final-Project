// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GYGameplayCueNotify_HitReaction.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGYGameplayCueNotify_HitReaction : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	virtual bool OnExecute_Implementation(AActor* MyTarget,	const FGameplayCueParameters& Parameters) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitCue|VFX")
	UMaterialInterface* OverlayMaterial;
};
