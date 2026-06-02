#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "Interaction/InteractionOption.h"
#include "GYInteractionPromptWidget.generated.h"

class UCommonTextBlock;

/**
 * 상호작용 시 뜨는 프롬프트 위젯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYInteractionPromptWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PromptText;

private:
	void HandleOptionsChanged(const TArray<FInteractionOption>& Options);
};
