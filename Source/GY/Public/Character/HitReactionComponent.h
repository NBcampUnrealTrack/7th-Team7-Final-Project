// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitReactionComponent.generated.h"


class UPhysicalAnimationComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitReactionComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ApplyHitReaction(const FVector& HitDirection, float Strength = -1.f, FName HitBone = NAME_None);

protected:
	TWeakObjectPtr<UPhysicalAnimationComponent> PhysicalAnimation;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	FName HitReactStartBone = TEXT("spine_01");

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	FName HitReactProfileName = TEXT("HitReaction");

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float HitReactDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float HitReactBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float HitReactBlendInWeight = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float DefaultHitImpulse = 1000.f;

	void EndHitReaction();

private:
	FTimerHandle HitReactTimerHandle;
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;

	bool bBlendingOut = false;
	float CurrentBlendWeight = 0.f;
};
