#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GYGameplayCueNotify_PostProcess.generated.h"

UCLASS()
class GY_API AGYGameplayCueNotify_PostProcess : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters) override;

	virtual bool OnRemove_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="PostProcess")
	TObjectPtr<UMaterialInterface> PostProcessMaterial;
};
