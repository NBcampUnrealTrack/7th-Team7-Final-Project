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

#pragma region HitReaction
public:
	virtual void BeginPlay() override;
	void ApplyHitReaction(const FVector& HitDirection, float Strength = -1.f, FName HitBone = NAME_None);

protected:
	TWeakObjectPtr<UPhysicalAnimationComponent> PhysicalAnimation;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	FName HitReactStartBone = TEXT("pelvis");

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	FName HitReactProfileName = TEXT("HitReaction");

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float HitReactDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float HitReactBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	float DefaultHitImpulse = 5000.f;

	void EndHitReaction();
	void FinishHitReactBlendOut();

private:
	FTimerHandle HitReactTimerHandle;
	FTimerHandle HitReactBlendOutTimerHandle;
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;
#pragma endregion
};
