// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestDamageZone.generated.h"
class UBoxComponent;
UCLASS()
class GY_API ATestDamageZone : public AActor
{
	GENERATED_BODY()

public:
	ATestDamageZone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyTickDamage();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Box;

	/** 한 틱당 가하는 raw damage (Defense는 ApplyDamage가 차감) */
	UPROPERTY(EditAnywhere, Category = "Damage")
	float DamagePerTick = 5.f;

	/** 데미지 인터벌(초) */
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMin = "0.05"))
	float TickInterval = 0.5f;

	/** 박스 크기 (반경) */
	UPROPERTY(EditAnywhere, Category = "Damage")
	FVector BoxExtent = FVector(200.f);

private:
	UPROPERTY()
	TSet<TObjectPtr<AActor>> OverlappingActors;

	FTimerHandle TickTimerHandle;
};
