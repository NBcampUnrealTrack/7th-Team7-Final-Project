#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PhantomStrike.generated.h"

// 팬텀 애니메이션(시퀀스)에 배치. 타격 프레임에 팬텀의 주변 판정을 실행
UCLASS()
class GY_API UPhantomStrike : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
