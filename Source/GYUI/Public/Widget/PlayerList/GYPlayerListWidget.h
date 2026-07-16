#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYPlayerListWidget.generated.h"

class APlayerState;
class UPanelWidget;
class UGYPlayerListEntryWidget;
struct FGYPartyMemberMessage;

/**
 * 접속한 다른 유저 표시하는 목록HUD 위젯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYPlayerListWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 유저가 추가될 목록 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> EntryContainer;

	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	TSubclassOf<UGYPlayerListEntryWidget> EntryWidgetClass;

	/** 본인도 포함할지 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	bool bIncludeLocalPlayer = false;

private:
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<APlayerState>, TObjectPtr<UGYPlayerListEntryWidget>> EntryMap;

	void AddPlayerEntry(APlayerState* PS);
	/** 약참조 키로 항목 제거 */
	void RemovePlayerEntry(const TWeakObjectPtr<APlayerState>& PSKey);

	/** PlayerState가 파괴됐지만 남아 있는 유령 항목 정리 */
	void PruneStaleEntries();

	void HandleMemberJoined(FGameplayTag Channel, const FGYPartyMemberMessage& Message);
	void HandleMemberLeft(FGameplayTag Channel, const FGYPartyMemberMessage& Message);

};
