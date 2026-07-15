#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionHighlightComponent.generated.h"

class UMaterialInterface;

// 상호작용 가능 범위 진입/이탈 시 대상 액터에 오버레이 머티리얼을 적용/해제
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UInteractionHighlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void SetHighlightedActor(AActor* NewTarget);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Highlight")
	TObjectPtr<UMaterialInterface> HighlightOverlayMaterial;

private:
	void ApplyOverlay(AActor* Target, UMaterialInterface* Material) const;

	TWeakObjectPtr<AActor> CurrentHighlightedActor;
};
