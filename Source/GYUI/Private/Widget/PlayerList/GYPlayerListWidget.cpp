#include "Widget/PlayerList/GYPlayerListWidget.h"
#include "GYUI/Public/Widget/PlayerList/GYPlayerListEntryWidget.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Core/GYUIManagerSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYPlayerListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 플레이어 입장, 퇴장 이벤트 구독
	ListenForMessage<UGYPlayerListWidget, FGYPartyMemberMessage>(
		GYGameplayTags::Message_Party_MemberJoined, this, &UGYPlayerListWidget::HandleMemberJoined);
	ListenForMessage<UGYPlayerListWidget, FGYPartyMemberMessage>(
		GYGameplayTags::Message_Party_MemberLeft, this, &UGYPlayerListWidget::HandleMemberLeft);

	// 위젯이 늦게 만들어진 경우 목록 re
	if (UGYUIManagerSubsystem* UI = UGYUIManagerSubsystem::Get(this))
	{
		APlayerState* LocalPS = GetOwningPlayerState();
		for (APlayerState* PS : UI->GetKnownPlayerStates())
		{
			if (!PS) continue;
			if (!bIncludeLocalPlayer && PS == LocalPS) continue; // 본인 제외
			AddPlayerEntry(PS);
		}
	}
}

void UGYPlayerListWidget::AddPlayerEntry(APlayerState* PS)
{
	if (!EntryContainer || !EntryWidgetClass) return;
	if (EntryMap.Contains(PS)) return;

	UGYPlayerListEntryWidget* Entry =
		CreateWidget<UGYPlayerListEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
	if (!Entry) return;

	EntryContainer->AddChild(Entry);
	Entry->InitializeFromPlayerState(PS);
	EntryMap.Add(PS, Entry);
}

void UGYPlayerListWidget::RemovePlayerEntry(const TWeakObjectPtr<APlayerState>& PSKey)
{
	if (TObjectPtr<UGYPlayerListEntryWidget>* Found = EntryMap.Find(PSKey))
	{
		if (UGYPlayerListEntryWidget* Entry = *Found)
		{
			Entry->RemoveFromParent();
		}
		EntryMap.Remove(PSKey);
	}
}

void UGYPlayerListWidget::PruneStaleEntries()
{
	for (auto It = EntryMap.CreateIterator(); It; ++It)
	{
		if (It.Key().IsValid()) continue; // PlayerState가 아직 살아 있으면 유지

		if (UGYPlayerListEntryWidget* Entry = It.Value())
		{
			Entry->RemoveFromParent();
		}
		It.RemoveCurrent();
	}
}

void UGYPlayerListWidget::HandleMemberJoined(FGameplayTag, const FGYPartyMemberMessage& Message)
{
	APlayerState* PS = Message.Member.Get();
	if (!PS) return;

	APlayerState* LocalPS = GetOwningPlayerState();
	if (!bIncludeLocalPlayer && PS == LocalPS) return;

	if (!EntryMap.Contains(PS))
	{
		AddPlayerEntry(PS);
	}
}

void UGYPlayerListWidget::HandleMemberLeft(FGameplayTag, const FGYPartyMemberMessage& Message)
{
	RemovePlayerEntry(Message.Member);
	PruneStaleEntries();
}
