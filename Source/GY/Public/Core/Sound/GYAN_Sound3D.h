#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GYAN_Sound3D.generated.h"

UCLASS()
class GY_API UGYAN_Sound3D : public UAnimNotify
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Sound")
	FGameplayTag SoundTag;

	UPROPERTY(EditAnywhere, Category = "Socket")
	FName AttachSocket = NAME_None;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
