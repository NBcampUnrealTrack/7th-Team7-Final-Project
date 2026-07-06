#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/BoxComponent.h"
#include "GYWeaponHitBox.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYWeaponHitBox : public UBoxComponent
{
	GENERATED_BODY()
public:
	UGYWeaponHitBox();

	void BeginHitDetection(AActor* InSource);
	void EndHitDetection();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
public:
	UPROPERTY(EditAnywhere, Category = "HitBox", meta = (Categories = "Weapon"))
	FGameplayTag SlotOrPartTag;

	UPROPERTY(EditAnywhere, Category = "HitBox")
	FName HitBoneName = TEXT("weapon");
private:
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> HitActors;
};
