#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYItemInfoWidget.generated.h"

class UImage;
class UCommonTextBlock;
class UDataTable;
struct FGYItemViewData;
struct FGYInventoryEntryMessage;
struct FRolledMagnitude;

// 아이템 정보 패널. Message.UI.ShowItemInfo 구독 → 우클릭한 아이템 정보 표시.
// 각 화면(인벤/루트/장비/인첸트)에 임베드해서 부모가 닫히면 함께 사라진다.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemInfoWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// pinned 모드(인첸트 대상 패널 등)에서 직접 아이템 표시. GMS 우클릭 흐름과 무관
	void ShowItem(const FGYItemViewData& Item);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// true면 공유 우클릭(ShowItemInfo) 메시지를 듣지 않고 ShowItem 직접 호출로만 갱신 (전용 패널용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|ItemInfo")
	bool bPinned = false;

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

	// MagnitudeTag → 표기 포맷/소수자리. 행 키 = MagnitudeTag 전체 이름 (DT_EnchantMagnitudeDisplay)
	UPROPERTY(EditDefaultsOnly, Category = "GY|ItemInfo")
	TSoftObjectPtr<UDataTable> MagnitudeDisplayTable;

	// 등급 색상 등 연출은 BP에서 (슬롯과 동일 패턴)
	UFUNCTION(BlueprintImplementableEvent, Category = "GY|ItemInfo")
	void OnItemInfoUpdated(FGameplayTag GradeTag);

private:
	void HandleShowItemInfo(FGameplayTag Channel, const FGYItemViewData& Item);
	// 표시 중인 아이템이 변경(리롤 등)되면 자동 갱신
	void HandleEntryChanged(FGameplayTag Channel, const FGYInventoryEntryMessage& Msg);
	// 실제 위젯 채우기 (토글/검증 없이)
	void ApplyView(const FGYItemViewData& Item);
	// 롤된 수치 1개를 MagnitudeDisplayTable 포맷으로 표기. 매핑 없으면 "+숫자" 폴백
	FText FormatMagnitude(const FRolledMagnitude& Magnitude, UDataTable* DisplayTable) const;

	// 현재 표시 중인 아이템의 출처 슬롯. 같은 소스 재우클릭 시 토글로 닫기
	TWeakObjectPtr<UObject> CurrentSource;
	// 표시 중인 인벤 아이템 id (EntryChanged 갱신 판정용)
	FGuid CurrentInstanceId;
	FGameplayMessageListenerHandle ListenerHandle;
	FGameplayMessageListenerHandle EntryListenerHandle;
};
