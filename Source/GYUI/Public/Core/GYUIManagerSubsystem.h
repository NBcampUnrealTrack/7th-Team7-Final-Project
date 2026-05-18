#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYUIManagerSubsystem.generated.h"

class UCommonActivatableWidget;
class UGYPrimaryGameLayout;

/**
 * 로컬마다 생성, 관리되는 UI 총괄 매니저
 */
UCLASS()
class GYUI_API UGYUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** PrimaryGameLayout 생성 후 화면 띄움 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void CreatePrimaryGameLayout(TSubclassOf<UGYPrimaryGameLayout> LayoutClass);

	/** 뷰포트에 추가된 레이아웃 제거, 참조 해제 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void RemovePrimaryGameLayout();

	/** 지정된 Layer 컨테이너에 위젯 추가 및 Activate */
	UFUNCTION(BlueprintCallable, Category = "GY|UI", meta = (DeterminesOutputType = "WidgetClass"))
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag,
	                                            TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 화면에 띄워진 위젯 레이어에서 제거 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void PopWidget(UCommonActivatableWidget* Widget);

	/** 현재 활성화된 PrimaryGameLayout 참조 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	UGYPrimaryGameLayout* GetPrimaryGameLayout() const { return PrimaryGameLayout; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<UGYPrimaryGameLayout> PrimaryGameLayout;
};
