#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GYConnectMenuWidget.generated.h"

class UButton;
class UEditableTextBox;

UCLASS()
class GYUI_API UGYConnectMenuWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYConnectMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> IpTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConnectButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connect")
	FString DefaultAddress = TEXT("127.0.0.1:7777");

private:
	UFUNCTION()
	void HandleConnectClicked();
};
