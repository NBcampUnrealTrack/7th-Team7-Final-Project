#pragma once

#include "CoreMinimal.h"
#include "Widget/MainMenu/GYSlidePanelWidget.h"
#include "GYSessionCreateWidget.generated.h"

class UButton;

UCLASS(Blueprintable)
class GYUI_API UGYSessionCreateWidget : public UGYSlidePanelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Create;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Cancel;

private:
	UFUNCTION() void HandleCancelClicked();
};
