#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotify_EnemyComboCancel.generated.h"

UCLASS()
class GY_API UAnimNotify_EnemyComboCancel : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;


	virtual FString GetNotifyName_Implementation() const override;

private:
	bool CancelExecution(USkeletalMeshComponent* MeshComp);
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
	float MaxHomingAngle = 20.f;

};
