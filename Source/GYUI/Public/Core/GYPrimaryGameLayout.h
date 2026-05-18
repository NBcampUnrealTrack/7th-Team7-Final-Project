#pragma once

#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYPrimaryGameLayout.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetContainerBase;

/**
 * 레이어 스택 컨테이너 위젯
 * UGYUIManagerSubsystem 통해 Push, Pop 할 것
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class GYUI_API UGYPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UGYPrimaryGameLayout(const FObjectInitializer& ObjectInitializer);

	/** 지정된 레이어에 새로운 위젯 띄움 */
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag,
	                                            TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 특정 레이어에서 위젯 지움 */
	void RemoveWidgetFromLayer(UCommonActivatableWidget* Widget);

	/** 태그를 주면 해당 태그에 연결된 스택 자체 반환 */
	UCommonActivatableWidgetContainerBase* GetLayerWidget(FGameplayTag LayerTag) const;

	/** UI 모두 날림 - 씬 전환 시 사용 */
	void ClearAllLayers();

protected:
	/** BP에서 직접 세팅 가능 - 태그, 이름 짝지어주기 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|Layers", meta = (Categories = "UI.Layer"))
	TMap<FGameplayTag, FName> LayerStackBindings;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;

	virtual void NativeOnInitialized() override;
};
