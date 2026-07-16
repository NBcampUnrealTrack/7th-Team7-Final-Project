#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceTarget.generated.h"

// 타겟(액터/벡터 키) 방향으로 회전하고, AcceptableAngle 이내에 들어오면 Succeeded
UCLASS()
class GY_API UBTTask_FaceTarget : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_FaceTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
private:
	bool GetTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;

	// 현재 Yaw가 타겟 방향과 이 각도(도) 이내면 성공 종료
	UPROPERTY(EditAnywhere, Category = "Rotation")
	float AcceptableAngle = 10.f;

	// 초당 회전 속도(도). RInterpConstantTo라 등속으로 돈다
	UPROPERTY(EditAnywhere, Category = "Rotation")
	float RotationSpeed = 360.f;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;
};
