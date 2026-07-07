#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "PoisonZoneActor.generated.h"

class USphereComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class GY_API APoisonZoneActor : public AActor
{
	GENERATED_BODY()
public:
	APoisonZoneActor();

	void Initialize(AActor* InInstigator);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> ZoneCollision;

	UPROPERTY(EditDefaultsOnly, Category = "PoisonZone",
		meta = (ToolTip = "장판의 반경 (cm)."))
	float ZoneRadius = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "PoisonZone",
		meta = (ToolTip = "장판이 유지되는 총 시간 (초)."))
	float Duration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "PoisonZone",
		meta = (ToolTip = "Periodic GE 한 번에 전달할 틱당 데미지 값."))
	float DamagePerTick = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "PoisonZone",
		meta = (ToolTip = "적용할 독 GE 클래스. GE_BossPoisonDoT 같은 Periodic GE."))
	TSubclassOf<UGameplayEffect> PoisonEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PoisonZone|Cue",
		meta = (Categories = "GameplayCue", ToolTip = "SFX 큐."))
	FGameplayTag ZoneSoundTag;

	UPROPERTY(EditDefaultsOnly, Category = "PoisonZone|Cue",
		meta = (Categories = "GameplayCue", ToolTip = "Post Process 큐."))
	FGameplayTag ZonePostProcessTag;

	UPROPERTY()
	TWeakObjectPtr<AActor> InstigatorActor;

	UPROPERTY()
	TMap<TObjectPtr<UAbilitySystemComponent>, FActiveGameplayEffectHandle> ActivePoisonHandles;
};
