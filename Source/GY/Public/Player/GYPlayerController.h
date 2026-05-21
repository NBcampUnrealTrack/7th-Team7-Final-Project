#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GYPlayerController.generated.h"

class UGYPrimaryGameLayout;
class UCommonActivatableWidget;
class AGYServerCheatProxy;

UCLASS()
class GY_API AGYPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGYPlayerController();
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;


	/** UI 레이아웃 클래스 정보 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|UI", meta=(AllowPrivateAccess=true))
	TSubclassOf<UGYPrimaryGameLayout> PrimaryGameLayoutClass;

	/** 기본 HUD 루트 위젯 클래스 정보 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|UI", meta=(AllowPrivateAccess=true))
	TSubclassOf<UCommonActivatableWidget> HUDWidgetClass;
public:

	UPROPERTY(Replicated)
	TObjectPtr<AGYServerCheatProxy> ServerCheatProxy;
};
