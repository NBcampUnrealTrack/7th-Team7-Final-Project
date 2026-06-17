#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbingComponent.generated.h"


class UCapsuleComponent;
class UAbilitySystemComponent;
class ALadder;
class AGYPlayerState;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClimbingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void BindToASC(AGYPlayerState* PlayerState);

protected:
	UFUNCTION()
	void OnCapsuleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnCapsuleOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool TryTriggerClimb();
	void TriggerClimbAbility(ALadder* Ladder);

	UPROPERTY(EditDefaultsOnly, Category="Climbing")
	float EntryDotThreshold = 0.7f;

	UPROPERTY()
	TArray<TWeakObjectPtr<ALadder>> CandidateLadders;

	TWeakObjectPtr<UPrimitiveComponent> LastEnteredBox;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UCapsuleComponent> BoundCapsule;
};
