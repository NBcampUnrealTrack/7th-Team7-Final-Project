#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestStatusZone.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class ETestStatusType : uint8
{
	Stun,
	Stagger,
};
UCLASS()
class GY_API ATestStatusZone : public AActor
{
	GENERATED_BODY()

public:
	ATestStatusZone();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(EditAnywhere, Category = "Test")
	ETestStatusType StatusType = ETestStatusType::Stun;

	UPROPERTY(EditAnywhere, Category = "Test")
	FVector BoxExtent = FVector(200.f);

	UPROPERTY(EditAnywhere, Category = "Test")
	bool bOneShot = true;

	/** 게임 화면에 박스 표시 */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugBox = true;

	/** 박스 색상 (Stun: Yellow, Stagger: Cyan 같은 식) */
	UPROPERTY(EditAnywhere, Category = "Debug")
	FColor DebugColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0.5"))
	float DebugLineThickness = 2.f;
};
