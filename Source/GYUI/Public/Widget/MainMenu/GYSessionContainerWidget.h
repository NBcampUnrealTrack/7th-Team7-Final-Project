#pragma once

#include "CoreMinimal.h"
#include "Widget/MainMenu/GYSlidePanelWidget.h"
#include "GYSessionContainerWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGYCreateSessionRequested);

UCLASS(Blueprintable)
class GYUI_API UGYSessionContainerWidget : public UGYSlidePanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GY|Session")
	FGYCreateSessionRequested OnCreateSessionRequested;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_CreateSession;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

private:
	UFUNCTION() void HandleCreateSessionClicked();
	UFUNCTION() void HandleBackClicked();
};
