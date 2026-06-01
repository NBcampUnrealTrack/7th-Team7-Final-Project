#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_AggroUpdate.generated.h"

struct FPerceivedActorInfo;

UCLASS()
class GY_API UBTService_AggroUpdate : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_AggroUpdate();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaSeconds) override;

private:
	float CalculateThreatScore(const FVector& EnemyLocation, const FPerceivedActorInfo& Info) const;

	UPROPERTY(EditAnywhere, Category = "Aggro")
	float BaseScore = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Aggro")
	float DistanceWeight = 1.f;
};
