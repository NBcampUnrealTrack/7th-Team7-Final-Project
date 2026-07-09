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

	virtual void Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed);
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

	UFUNCTION()
	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float SweepRadius = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float MaxLifeTime = 3.f;

	/** Projectile 적중시 Tag */
	UPROPERTY(EditDefaultsOnly, Category = "Cue")
	FGameplayTag HitCueTag;

	/** Projectile 효과음 */
	UPROPERTY(EditDefaultsOnly, Category = "Cue")
	FGameplayTag SoundCueTag;

	UPROPERTY()
	TWeakObjectPtr<AActor> InstigatorActor;

};
