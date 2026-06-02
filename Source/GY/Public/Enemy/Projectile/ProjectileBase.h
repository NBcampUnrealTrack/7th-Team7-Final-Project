#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS(Abstract)
class GY_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();

	void Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed);
protected:
	virtual  void BeginPlay() override;

	UFUNCTION()
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,
									  AActor* OtherActor,
									  UPrimitiveComponent* OtherComponent,
									  int32 OtherBodyIndex,
									  bool bFromSweep,
									  const FHitResult& SweepResult);

	virtual void OnHitTarget(AActor* HitActor, const FHitResult& HitResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float SweepRadius = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float MaxLifeTime = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag;

	UPROPERTY()
	TWeakObjectPtr<AActor> InstigatorActor;

};
