#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYPlayerHUDWidget.generated.h"

class UGYEquipSlotWidget;
class UGYPotionSlotWidget;
class UGYStatBarWidget;
class UCommonTextBlock;
class UCommonRichTextBlock;
struct FGYPlayerNameMessage;
struct FGYXPProgressMessage;
class UProgressBar;

/**
 * HUD 최상위 위젯 - 플레이어 설정값 모아운 묶음
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYPlayerHUDWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYStatBarWidget> StatBar_HP;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYStatBarWidget> StatBar_Stamina;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> Bar_XP;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYPotionSlotWidget> PotionSlot_HP;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYPotionSlotWidget> PotionSlot_Stamina;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYEquipSlotWidget> EquipSlot_Weapon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYEquipSlotWidget> EquipSlot_Outfit;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_PlayerName;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Level;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonRichTextBlock> Text_XP;

	/** 레벨 출력 형식 지정 ("Lv. {0}") */
	UPROPERTY(EditDefaultsOnly, Category = "GY|HUD")
	FText LevelFormat;
	/** 경험치 출력 형식 지정 ("{0} / {1}") */
	UPROPERTY(EditDefaultsOnly, Category = "GY|HUD")
	FText XPFormat;

private:
	void HandlePlayerNameMessage(FGameplayTag Channel, const FGYPlayerNameMessage& Message);
	void HandleXPProgressMessage(FGameplayTag Channel, const FGYXPProgressMessage& Message);
};
