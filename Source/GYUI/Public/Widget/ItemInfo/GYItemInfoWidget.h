#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYItemInfoWidget.generated.h"

class UImage;
class UCommonTextBlock;
struct FGYItemViewData;

// 아이템 정보 패널. Message.UI.ShowItemInfo 구독 → 우클릭한 아이템 정보 표시.
// 각 화면(인벤/루트/장비/인첸트)에 임베드해서 부모가 닫히면 함께 사라진다.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemInfoWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Grade;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Level;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Description;

	// 인첸트 옵션 줄(완성형). 여러 줄을 개행으로 합쳐 표시
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_EnchantOptions;

	// 등급 색상 등 연출은 BP에서 (슬롯과 동일 패턴)
	UFUNCTION(BlueprintImplementableEvent, Category = "GY|ItemInfo")
	void OnItemInfoUpdated(FGameplayTag GradeTag);

private:
	void HandleShowItemInfo(FGameplayTag Channel, const FGYItemViewData& Item);

	FGameplayMessageListenerHandle ListenerHandle;
};
