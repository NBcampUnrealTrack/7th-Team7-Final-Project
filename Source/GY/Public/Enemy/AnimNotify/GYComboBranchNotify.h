#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GYComboBranchNotify.generated.h"

UCLASS()
class GY_API UGYComboBranchNotify : public UAnimNotify
{
	GENERATED_BODY()
public:
	UGYComboBranchNotify();

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
