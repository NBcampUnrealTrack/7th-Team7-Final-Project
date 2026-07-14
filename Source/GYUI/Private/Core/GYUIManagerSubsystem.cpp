#include "Core/GYUIManagerSubsystem.h"
#include "CoreGlobals.h"
#include "GYUI/Public/Core/GYPrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Core/GYUISettings.h"
#include "GameplayTags/GYUILayerTags.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Blueprint/UserWidget.h"
#include "GY/Public/Player/GYPlayerController.h"
#include "GY/Public/Player/GYPlayerState.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Core/Sound/GYSoundManager.h"
#include "UI/GYUIMessages.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/GameStateBase.h"
#include "Character/GYPawnData.h"
#include "Character/GYPlayerActionConfig.h"
#include "Widget/EndingCredits/GYEndingCreditsWidget.h"
#include "Widget/EndingCredits/GYEndingNarrativeWidget.h"
#include "Widget/Interaction/GYInteractionWaitingWidget.h"
#include "Widget/WorldReset/GYWorldResetWidget.h"
#include "Enemy/GYEnemyCharacterBase.h"

void UGYUIManagerSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RosterSyncHandle);
		World->GetTimerManager().ClearTimer(NarrativeCleanupTimerHandle);
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MSG = GI->GetSubsystem<UGameplayMessageSubsystem>())
			{
				MSG->UnregisterListener(RegionEnterListenerHandle);
				MSG->UnregisterListener(RegionExitListenerHandle);
				MSG->UnregisterListener(EndingStartedHandle);
				MSG->UnregisterListener(EndingWaitingHandle);
				MSG->UnregisterListener(EndingCinematicFinishedHandle);
				MSG->UnregisterListener(EndingNarrativeFinishedHandle);
				MSG->UnregisterListener(EndingCreditsFinishedHandle);
				MSG->UnregisterListener(ClockOverlayHandle);
				MSG->UnregisterListener(BossStateListenerHandle);
				MSG->UnregisterListener(ToggleSettingsListenerHandle);
				MSG->UnregisterListener(EnterCinematicHandle);
				MSG->UnregisterListener(ReviveHoldHandle);
			}
		}
		UnbindASC();
	}
	RemovePrimaryGameLayout();
	Super::Deinitialize();
}

bool UGYUIManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	if (IsRunningDedicatedServer()) return false;
	return true;
}

void UGYUIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (!NewPlayerController || !NewPlayerController->IsLocalController()) return;

	if (const UGYUISettings* Settings = GetDefault<UGYUISettings>())
	{
		if (UClass* LayoutClass = Settings->PrimaryGameLayoutClass.LoadSynchronous())
		{
			CreatePrimaryGameLayout(LayoutClass);
		}
		if (UClass* HUDClass = Settings->HUDWidgetClass.LoadSynchronous())
		{
			// 레이아웃이 생성되면 새로 push, 같은 레이아웃에 이미 떠 있으면 건너뜀
			if (!HUDWidget.IsValid())
			{
				HUDWidget = PushWidgetToLayer(GYUILayerTags::UI_Layer_Game, HUDClass);
			}
		}
	}

	if (AGYPlayerState* PS = NewPlayerController->GetPlayerState<AGYPlayerState>())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			BindASC(ASC);
		}
	}

	if (AGYPlayerController* GYPC = Cast<AGYPlayerController>(NewPlayerController))
	{
		GYPC->OnPlayerStateInitialized.RemoveAll(this); // 기존 바인딩 해제
		GYPC->OnPlayerStateInitialized.AddUObject(this, &UGYUIManagerSubsystem::HandlePlayerStateInitialized);

		GYPC->OnLocalControllerReady.RemoveAll(this);
		GYPC->OnLocalControllerReady.AddDynamic(this, &UGYUIManagerSubsystem::HandleLocalControllerReady);
	}

	// 플레이어 명단 동기화
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(RosterSyncHandle))
		{
			World->GetTimerManager().SetTimer(RosterSyncHandle, FTimerDelegate::CreateUObject(
				this, &UGYUIManagerSubsystem::SyncPlayerRoster), RosterSyncInterval, true);
		}
		SyncPlayerRoster();
	}

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& MSG = UGameplayMessageSubsystem::Get(World);

		// 등록 리스너 해제, 바인딩 중복 방지
		if (RegionEnterListenerHandle.IsValid()) { MSG.UnregisterListener(RegionEnterListenerHandle); }
		if (RegionExitListenerHandle.IsValid()) { MSG.UnregisterListener(RegionExitListenerHandle); }
		if (EndingStartedHandle.IsValid()) { MSG.UnregisterListener(EndingStartedHandle); }
		if (EndingWaitingHandle.IsValid()) { MSG.UnregisterListener(EndingWaitingHandle); }
		if (EndingCinematicFinishedHandle.IsValid()) { MSG.UnregisterListener(EndingCinematicFinishedHandle); }
		if (EndingNarrativeFinishedHandle.IsValid()) {MSG.UnregisterListener(EndingNarrativeFinishedHandle);}
		if (EndingCreditsFinishedHandle.IsValid()) { MSG.UnregisterListener(EndingCreditsFinishedHandle); }
		if (ClockOverlayHandle.IsValid()) { MSG.UnregisterListener(ClockOverlayHandle); }
		if (BossStateListenerHandle.IsValid()) { MSG.UnregisterListener(BossStateListenerHandle); }
		if (ToggleSettingsListenerHandle.IsValid()) { MSG.UnregisterListener(ToggleSettingsListenerHandle); }
		if (EnterCinematicHandle.IsValid()) { MSG.UnregisterListener(EnterCinematicHandle); }
		if (ReviveHoldHandle.IsValid()) { MSG.UnregisterListener(ReviveHoldHandle); }

		RegionEnterListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Region_Entered, this, &UGYUIManagerSubsystem::HandleRegionEntered);
		RegionExitListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Region_Exited, this, &UGYUIManagerSubsystem::HandleRegionExited);
		EndingStartedHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_Started, this, &UGYUIManagerSubsystem::HandleEndingStarted);
		EndingWaitingHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_WaitingForPlayers, this, &UGYUIManagerSubsystem::HandleEndingWaiting);
		EndingCinematicFinishedHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_CinematicFinished, this, &UGYUIManagerSubsystem::HandleEndingCinematicFinished);
		EndingNarrativeFinishedHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_NarrativeFinished, this, &UGYUIManagerSubsystem::HandleEndingNarrativeFinished);
		EndingCreditsFinishedHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_CreditsFinished, this, &UGYUIManagerSubsystem::HandleEndingCreditsFinished);
		ClockOverlayHandle = MSG.RegisterListener(
			GYGameplayTags::Message_UI_ClockOverlay, this, &UGYUIManagerSubsystem::HandleClockOverlay);
		BossStateListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Boss_State, this, &UGYUIManagerSubsystem::HandleBossState);
		ToggleSettingsListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_UI_ToggleSettings, this, &UGYUIManagerSubsystem::HandleToggleSettings);
		EnterCinematicHandle = MSG.RegisterListener(
			   GYGameplayTags::Message_Cinematic_State, this, &UGYUIManagerSubsystem::HandleEnterCinematic);
		ReviveHoldHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Player_ReviveHold, this, &UGYUIManagerSubsystem::HandleReviveHold);
	}
}

void UGYUIManagerSubsystem::HandlePlayerStateInitialized(AGYPlayerController* PC)
{
	if (!PC) return;
	EnsurePrimaryLayoutAndHUD();
	AGYPlayerState* PS = PC->GetPlayerState<AGYPlayerState>();
	if (!PS) return;
	if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
	{
		BindASC(ASC);
	}
}

void UGYUIManagerSubsystem::HandleLocalControllerReady(AGYPlayerController* PC)
{
	EnsurePrimaryLayoutAndHUD();
}

void UGYUIManagerSubsystem::EnsurePrimaryLayoutAndHUD()
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;
	APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
	if (!PC || !PC->IsLocalController()) return;

	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	if (!Settings) return;

	if (UClass* LayoutClass = Settings->PrimaryGameLayoutClass.LoadSynchronous())
	{
		CreatePrimaryGameLayout(LayoutClass);
	}
	if (UClass* HUDClass = Settings->HUDWidgetClass.LoadSynchronous())
	{
		if (!HUDWidget.IsValid())
		{
			HUDWidget = PushWidgetToLayer(GYUILayerTags::UI_Layer_Game, HUDClass);
		}
	}
}

void UGYUIManagerSubsystem::RegisterStatBroadcast(UAbilitySystemComponent* ASC)
{
	UnregisterStatBroadcast();
	StatBroadcastEntries.Reset();

	auto Add = [&](FGameplayTag Channel, const FGameplayAttribute& Cur, const FGameplayAttribute& Max)
	{
		FStatBroadcastEntry Entry;
		Entry.Channel = Channel;
		Entry.CurrentAttribute = Cur;
		Entry.MaxAttribute = Max;

		// Attribute 변동 시 함수 호출 요청
		Entry.CurrentHandle = ASC->GetGameplayAttributeValueChangeDelegate(Cur).AddUObject(
			this, &UGYUIManagerSubsystem::OnStatAttributeChanged);
		Entry.MaxHandle = ASC->GetGameplayAttributeValueChangeDelegate(Max).AddUObject(
			this, &UGYUIManagerSubsystem::OnStatAttributeChanged);

		StatBroadcastEntries.Add(MoveTemp(Entry));
	};

	// 속성 매핑해서 등록
	Add(GYGameplayTags::Message_Stat_Health, UGYVitalAttributeSet::GetCurrentHealthAttribute(),
	    UGYVitalAttributeSet::GetMaxHealthAttribute());
	Add(GYGameplayTags::Message_Stat_Stamina, UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute(),
	    UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute());
	Add(GYGameplayTags::Message_Stat_Poise, UGYVitalAttributeSet::GetCurrentStunAttribute(),
	    UGYVitalAttributeSet::GetMaxStunAttribute());

	LevelHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGYProgressionAttributeSet::GetLevelAttribute()).AddUObject(
		this, &UGYUIManagerSubsystem::OnXPRelatedChanged);
	XPHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGYProgressionAttributeSet::GetXPAttribute()).AddUObject(
		this, &UGYUIManagerSubsystem::OnXPRelatedChanged);

	// 연동 후 현재 값들 바로 전달
	for (const FStatBroadcastEntry& Entry : StatBroadcastEntries)
	{
		BroadcastStat(ASC, Entry);
	}
	BroadcastXP();
}

void UGYUIManagerSubsystem::UnregisterStatBroadcast()
{
	if (BoundASC.IsValid())
	{
		for (const FStatBroadcastEntry& Entry : StatBroadcastEntries)
		{
			BoundASC->GetGameplayAttributeValueChangeDelegate(Entry.CurrentAttribute).Remove(Entry.CurrentHandle);
			BoundASC->GetGameplayAttributeValueChangeDelegate(Entry.MaxAttribute).Remove(Entry.MaxHandle);
		}
		BoundASC->GetGameplayAttributeValueChangeDelegate(UGYProgressionAttributeSet::GetLevelAttribute()).Remove(LevelHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UGYProgressionAttributeSet::GetXPAttribute()).Remove(XPHandle);
	}
	StatBroadcastEntries.Reset();
	LevelHandle.Reset();
	XPHandle.Reset();
}

void UGYUIManagerSubsystem::OnStatAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (!BoundASC.IsValid()) return;

	// 바뀐 스탯 찾아서 해당 채널로 방송
	for (const FStatBroadcastEntry& Entry : StatBroadcastEntries)
	{
		if (Data.Attribute == Entry.CurrentAttribute || Data.Attribute == Entry.MaxAttribute)
		{
			BroadcastStat(BoundASC.Get(), Entry);
			return;
		}
	}
}

void UGYUIManagerSubsystem::BroadcastStat(UAbilitySystemComponent* ASC, const FStatBroadcastEntry& Entry)
{
	if (!ASC || !GetWorld()) return;

	FGYAttributeValueMessage Msg;
	Msg.CurrentValue = ASC->GetNumericAttribute(Entry.CurrentAttribute);
	Msg.MaxValue = ASC->GetNumericAttribute(Entry.MaxAttribute);

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(Entry.Channel, Msg);
}

void UGYUIManagerSubsystem::BroadcastXP()
{
	if (!BoundASC.IsValid() || !GetWorld()) return;

	const UGYProgressionAttributeSet* PA = BoundASC->GetSet<UGYProgressionAttributeSet>();
	if (!PA) return;

	FGYXPProgressMessage Msg;
	Msg.Level = FMath::RoundToInt(PA->GetLevel());
	Msg.CurrentXP = PA->GetXP();

	Msg.MaxXP = PA->NextLevelXPCurve ? PA->NextLevelXPCurve->GetFloatValue(PA->GetLevel()) : 1.f;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_UI_XPProgress, Msg);
}

void UGYUIManagerSubsystem::OnXPRelatedChanged(const FOnAttributeChangeData& Data)
{
	BroadcastXP();
}

void UGYUIManagerSubsystem::BindASC(UAbilitySystemComponent* InASC)
{
	if (!InASC || BoundASC == InASC) return;

	UnbindASC();
	BoundASC = InASC;

	// 등록된 태그 전부 재구독
	for (auto& Pair : TagWidgetMap)
	{
		Pair.Value.DelegateHandle =
			InASC->RegisterGameplayTagEvent(Pair.Key, EGameplayTagEventType::NewOrRemoved)
				 .AddUObject(this, &UGYUIManagerSubsystem::OnTagChanged);
	}
	RegisterStatBroadcast(InASC);

	// 부활 표현 - 시계 오버레이
	DeathTagHandle = InASC->RegisterGameplayTagEvent(GYStateTags::State_Life_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddWeakLambda(this, [this](const FGameplayTag, int32 NewCount)
		{
			if (NewCount > 0)
			{
				StopGiveUpProgress();
				UpdateRevivalWidget();
				PlayClockOverlay(0.f, GYStateTags::State_Life_Dead);
			}
			else
			{
				RequestStopClockOverlay();
			}
		});

	// 부활 진행 위젯 - 부활 시전자/대상 태그 감지
	RevivingTagHandle = InASC->RegisterGameplayTagEvent(GYStateTags::State_Action_Reviving, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGYUIManagerSubsystem::HandleRevivalTagChanged);
	BeingRevivedTagHandle = InASC->RegisterGameplayTagEvent(GYStateTags::State_Life_BeingRevived, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGYUIManagerSubsystem::HandleRevivalTagChanged);

	for (const auto& Pair : TagWidgetMap)
	{
		const int32 Count = InASC->GetTagCount(Pair.Key);
		if (Count > 0) OnTagChanged(Pair.Key, Count);
	}

	if (InASC->GetTagCount(GYStateTags::State_Life_Dead) > 0)
	{
		PlayClockOverlay(0.f, GYStateTags::State_Life_Dead);
	}

	UpdateRevivalWidget();

	bGameplayInputBlockApplied = false;
	RefreshGameplayInputBlock();
}

void UGYUIManagerSubsystem::RegisterTagDrivenWidget(
	FGameplayTag StateTag,
	FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass,
	bool bBlocksOtherWidgets)
{
	// 똑같은 태그 존재 시 기존 구독 취소
	if (FTagWidgetEntry* ExistingEntry = TagWidgetMap.Find(StateTag))
	{
		if (BoundASC.IsValid() && ExistingEntry->DelegateHandle.IsValid())
		{
			BoundASC->UnregisterGameplayTagEvent(ExistingEntry->DelegateHandle, StateTag,
			                                     EGameplayTagEventType::NewOrRemoved);
		}
	}

	FTagWidgetEntry Entry;
	Entry.LayerTag = LayerTag;
	Entry.WidgetClass = WidgetClass;
	Entry.bBlocksOtherWidgets = bBlocksOtherWidgets;

	if (BoundASC.IsValid())
	{
		Entry.DelegateHandle =
			BoundASC->RegisterGameplayTagEvent(StateTag, EGameplayTagEventType::NewOrRemoved)
			        .AddUObject(this, &UGYUIManagerSubsystem::OnTagChanged);
	}

	TagWidgetMap.Add(StateTag, Entry);
}

void UGYUIManagerSubsystem::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	FTagWidgetEntry* Entry = TagWidgetMap.Find(Tag);
	if (!Entry)	return;

	//위젯push
	if (NewCount > 0)
	{
		if (!Entry->ActiveWidget.IsValid())
		{
			UCommonActivatableWidget* Widget = Entry->bBlocksOtherWidgets
				? PushSystemWidget(Entry->LayerTag, Entry->WidgetClass)
				: PushWidgetRaw(Entry->LayerTag, Entry->WidgetClass);
			Entry->ActiveWidget = Widget;
		}
	}
	else //위젯 pop
	{
		if (Entry->ActiveWidget.IsValid())
		{
			PopWidget(Entry->ActiveWidget.Get());
			Entry->ActiveWidget = nullptr;
		}
	}
}

void UGYUIManagerSubsystem::CreatePrimaryGameLayout(TSubclassOf<UGYPrimaryGameLayout> LayoutClass)
{
	if (!LayoutClass) return;

	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;
	APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
	if (!PC) return;

	// 현재 PlayerController 소유의 레이아웃이 이미 살아 있으면 재생성x
	// if (PrimaryGameLayout && PrimaryGameLayout->GetOwningPlayer() == PC)
	// {
	// 	return;
	// }
	RemovePrimaryGameLayout();

	PrimaryGameLayout = CreateWidget<UGYPrimaryGameLayout>(PC, LayoutClass);
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->AddToPlayerScreen(1000); // 항상 최상단 ZOrder
	}
}

void UGYUIManagerSubsystem::RemovePrimaryGameLayout()
{
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->RemoveFromParent(); // 뷰포트 제거
		PrimaryGameLayout = nullptr;
	}
	HUDWidget = nullptr; // 레이아웃과 함께 사라지는 HUD 참조도 초기화
}

UCommonActivatableWidget* UGYUIManagerSubsystem::PushWidgetRaw(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryGameLayout) return nullptr;
	return PrimaryGameLayout->PushWidgetToLayer(LayerTag, WidgetClass);
}

UCommonActivatableWidget* UGYUIManagerSubsystem::PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (LayerTag != GYUILayerTags::UI_Layer_Menu)
	{
		return PushWidgetRaw(LayerTag, WidgetClass);
	}
	if (IsSystemUIActive())
	{
		return nullptr;
	}
	CloseAllMenus();

	UCommonActivatableWidget* Widget = PushWidgetRaw(LayerTag, WidgetClass);
	TrackMenuWidget(Widget);
	return Widget;
}

UCommonActivatableWidget* UGYUIManagerSubsystem::PushSystemWidget(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	CloseAllMenus();
	UCommonActivatableWidget* Widget = PushWidgetRaw(LayerTag, WidgetClass);
	if (Widget)
	{
		ActiveSystemWidgets.Add(Widget);
		RefreshGameplayInputBlock();
	}
	return Widget;
}

void UGYUIManagerSubsystem::TrackMenuWidget(UCommonActivatableWidget* Widget)
{
	if (!Widget) return;

	ActiveMenuWidgets.Add(Widget);
	RefreshGameplayInputBlock(); //이동, 공격 차단

	Widget->OnDeactivated().AddWeakLambda(this, [this, Widget]()
	{
		ActiveMenuWidgets.RemoveAll([Widget](const TWeakObjectPtr<UCommonActivatableWidget>& Entry)
		{
			return !Entry.IsValid() || Entry.Get() == Widget;
		});
		RefreshGameplayInputBlock(); // 마지막 메뉴가 닫히면 차단 해제
	});
}

void UGYUIManagerSubsystem::CloseAllMenus()
{
	if (ActiveMenuWidgets.Num() == 0) return;

	TArray<TWeakObjectPtr<UCommonActivatableWidget>> Snapshot = MoveTemp(ActiveMenuWidgets);
	ActiveMenuWidgets.Reset();

	for (const TWeakObjectPtr<UCommonActivatableWidget>& Entry : Snapshot)
	{
		if (Entry.IsValid())
		{
			PopWidget(Entry.Get());
		}
	}

	RefreshGameplayInputBlock();
}

bool UGYUIManagerSubsystem::HasBlockingSystemWidget() const
{
	if (ActiveUIBlockReasons.Num() > 0) return true; // 시네마틱/네러티브
	for (const TWeakObjectPtr<UCommonActivatableWidget>& W : ActiveSystemWidgets)
	{
		if (W.IsValid()) return true; // 시간의 틈, 스턴, 세계리셋, 엔딩
	}
	return false;
}

bool UGYUIManagerSubsystem::ShouldBlockGameplayInput() const
{
	return ActiveMenuWidgets.Num() > 0 || HasBlockingSystemWidget();
}

void UGYUIManagerSubsystem::RefreshGameplayInputBlock()
{
	if (!BoundASC.IsValid()) return;

	const bool bShouldBlock = ShouldBlockGameplayInput();
	if (bShouldBlock == bGameplayInputBlockApplied) return;

	bShouldBlock ? BoundASC->AddLooseGameplayTag(GYStateTags::State_UI_MenuOpen)
	             : BoundASC->RemoveLooseGameplayTag(GYStateTags::State_UI_MenuOpen);
	bGameplayInputBlockApplied = bShouldBlock;
}

bool UGYUIManagerSubsystem::IsSystemUIActive() const
{
	if (ActiveRevivalWidget.IsValid() || bReviveHoldActive) return true;
	return HasBlockingSystemWidget();
}

void UGYUIManagerSubsystem::PopWidget(UCommonActivatableWidget* Widget)
{
	if (PrimaryGameLayout && Widget)
	{
		PrimaryGameLayout->RemoveWidgetFromLayer(Widget);
	}
	if (Widget)
	{
		const int32 Removed = ActiveSystemWidgets.RemoveAll([Widget](const TWeakObjectPtr<UCommonActivatableWidget>& E)
		{
			return !E.IsValid() || E.Get() == Widget;
		});
		if (Removed > 0) RefreshGameplayInputBlock();
	}
}

UGYUIManagerSubsystem* UGYUIManagerSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World) return nullptr;

	// 첫 번째 로컬 플레이어 기준
	const ULocalPlayer* LP = World->GetFirstLocalPlayerFromController();
	return LP ? LP->GetSubsystem<UGYUIManagerSubsystem>() : nullptr;
}

FString UGYUIManagerSubsystem::GetPlayerName(APlayerState* PS) const
{
	if (!PS) return FString();

	if (const FString* Cached = KnownPlayerNames.Find(TWeakObjectPtr<APlayerState>(PS)))
	{
		return *Cached;
	}
	return PS->GetPlayerName(); // 아직 동기화 안 된 시점에 호출된 경우 - 캐시 미스 시 폴백
}

TArray<APlayerState*> UGYUIManagerSubsystem::GetKnownPlayerStates() const
{
	TArray<APlayerState*> Result;
	Result.Reserve(KnownPlayerNames.Num());
	for (const auto& Pair : KnownPlayerNames)
	{
		if (APlayerState* PS = Pair.Key.Get())
		{
			Result.Add(PS);
		}
	}
	return Result;
}

void UGYUIManagerSubsystem::UnbindASC()
{
	if (BoundASC.IsValid())
	{
		// 델리게이트 해제
		for (auto& Pair : TagWidgetMap)
		{
			if (Pair.Value.DelegateHandle.IsValid()) // 이벤트 구독 해제
			{
				BoundASC->UnregisterGameplayTagEvent(Pair.Value.DelegateHandle, Pair.Key,
				                                     EGameplayTagEventType::NewOrRemoved);
				Pair.Value.DelegateHandle.Reset();
			}
			// 남아 있는 위젯 닫기
			if (Pair.Value.ActiveWidget.IsValid())
			{
				PopWidget(Pair.Value.ActiveWidget.Get());
				Pair.Value.ActiveWidget = nullptr;
			}
		}

		if (DeathTagHandle.IsValid())
		{
			BoundASC->UnregisterGameplayTagEvent(DeathTagHandle, GYStateTags::State_Life_Dead,
			                                     EGameplayTagEventType::NewOrRemoved);
			DeathTagHandle.Reset();
		}

		if (RevivingTagHandle.IsValid())
		{
			BoundASC->UnregisterGameplayTagEvent(RevivingTagHandle, GYStateTags::State_Action_Reviving,
												 EGameplayTagEventType::NewOrRemoved);
			RevivingTagHandle.Reset();
		}

		if (BeingRevivedTagHandle.IsValid())
		{
			BoundASC->UnregisterGameplayTagEvent(BeingRevivedTagHandle, GYStateTags::State_Life_BeingRevived,
												 EGameplayTagEventType::NewOrRemoved);
			BeingRevivedTagHandle.Reset();
		}

		StopGiveUpProgress();
		if (ActiveRevivalWidget.IsValid())
		{
			ActiveRevivalWidget->RemoveFromParent();
			ActiveRevivalWidget = nullptr;
		}

		StopClockOverlay();
		UnregisterStatBroadcast();

		// 이 ASC에 붙였던 UI 차단 태그 정리
		if (bGameplayInputBlockApplied)
		{
			BoundASC->RemoveLooseGameplayTag(GYStateTags::State_UI_MenuOpen);
			bGameplayInputBlockApplied = false;
		}
	}
	BoundASC = nullptr;
}

APlayerState* UGYUIManagerSubsystem::GetLocalPlayerState() const
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return nullptr;
	APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
	return PC ? PC->PlayerState : nullptr;
}

void UGYUIManagerSubsystem::SyncPlayerRoster()
{
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS) return;

	APlayerState* LocalPS = GetLocalPlayerState();
	UGameplayMessageSubsystem& Msg = UGameplayMessageSubsystem::Get(GetWorld());

	TSet<TWeakObjectPtr<APlayerState>> CurrentSet;

	for (APlayerState* PS : GS->PlayerArray) // 게임 내 모든 플레이어 순회
	{
		if (!PS || PS->IsInactive()) continue;

		const TWeakObjectPtr<APlayerState> Key(PS);
		CurrentSet.Add(Key);

		const FString CurrentName = PS->GetPlayerName();
		const FString* Existing = KnownPlayerNames.Find(Key);
		const bool bIsNew = (Existing == nullptr); // 캐시에 없으면 신규 유저

		if (bIsNew || *Existing != CurrentName) // 캐시에 없음, 이름 변경된 경우 브로드캐스트
		{
			KnownPlayerNames.Add(Key, CurrentName);

			FGYPlayerNameMessage NamePayload;
			NamePayload.PlayerState = PS;
			NamePayload.PlayerName = CurrentName;
			NamePayload.bIsLocalPlayer = (PS == LocalPS);
			Msg.BroadcastMessage(GYGameplayTags::Message_UI_PlayerName, NamePayload); // 이름 업뎃
		}

		// 신규 입장 브로드캐스트
		if (bIsNew)
		{
			FGYPartyMemberMessage JoinPayload;
			JoinPayload.Member = PS;
			JoinPayload.EventTag = GYGameplayTags::Message_Party_MemberJoined;
			Msg.BroadcastMessage(GYGameplayTags::Message_Party_MemberJoined, JoinPayload);
		}
	}

	// 퇴장 브로드캐스트, 캐시 정리
	for (auto It = KnownPlayerNames.CreateIterator(); It; ++It)
	{
		if (!CurrentSet.Contains(It.Key()))
		{
			FGYPartyMemberMessage LeavePayload;
			LeavePayload.Member = It.Key();
			LeavePayload.EventTag = GYGameplayTags::Message_Party_MemberLeft;
			Msg.BroadcastMessage(GYGameplayTags::Message_Party_MemberLeft, LeavePayload);

			It.RemoveCurrent();
		}
	}
}

UCommonActivatableWidget* UGYUIManagerSubsystem::ToggleWidgetInLayer(
	FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (WidgetClass == nullptr) return nullptr;

	TWeakObjectPtr<UCommonActivatableWidget>& Existing = ToggleWidgetMap.FindOrAdd(WidgetClass);
	if (Existing.IsValid())
	{
		PopWidget(Existing.Get());
		Existing = nullptr;
		return nullptr;
	}

	UCommonActivatableWidget* Widget = PushWidgetToLayer(LayerTag, WidgetClass);
	if (Widget != nullptr)
	{
		Existing = Widget;
		Widget->OnDeactivated().AddLambda([WeakThis = MakeWeakObjectPtr(this), WidgetClass]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ToggleWidgetMap.Remove(WidgetClass);
			}
		});
	}
	return Widget;
}

void UGYUIManagerSubsystem::HandleRegionEntered(FGameplayTag, const FGYRegionEnteredMessage& Msg)
{
	ULocalPlayer* LP = GetLocalPlayer();
	APlayerController* LocalPC = LP ? LP->GetPlayerController(GetWorld()) : nullptr;
	APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;
	if (!LocalPawn || Msg.Pawn.Get() != LocalPawn) return;

	ActiveRegionId = Msg.RegionId;
}

void UGYUIManagerSubsystem::HandleRegionExited(FGameplayTag Tag, const FGYRegionExitedMessage& Msg)
{
	ULocalPlayer* LP = GetLocalPlayer();
	APlayerController* LocalPC = LP ? LP->GetPlayerController(GetWorld()) : nullptr;
	APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;
    if (Msg.Pawn.Get() != LocalPawn) return;

	if (Msg.RegionId == ActiveRegionId)
	{
		ActiveRegionId = FGameplayTag(); // 지역 정보 초기화
	}
}

void UGYUIManagerSubsystem::HandleClockOverlay(FGameplayTag, const FGYClockOverlayMessage& Msg)
{
	PlayClockOverlay(Msg.HoldDuration, Msg.Reason);
}

void UGYUIManagerSubsystem::HandleBossState(FGameplayTag, const FGYBossStateMessage& Msg)
{
	bBossFightActive = Msg.bVisible;
}

void UGYUIManagerSubsystem::PlayClockOverlay(float HoldDuration, FGameplayTag Reason)
{
	if (bBossFightActive && Reason != GYStateTags::State_Life_Dead)
	{
		return;
	}

	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	UClass* WidgetClass = Settings ? Settings->WorldResetWidgetClass.LoadSynchronous() : nullptr;
	if (!WidgetClass) return;

	StopClockOverlay();

	UCommonActivatableWidget* W = PushSystemWidget(GYUILayerTags::UI_Layer_Menu, WidgetClass);
	UGYWorldResetWidget* Overlay = Cast<UGYWorldResetWidget>(W);
	if (!Overlay) return;

	ActiveClockOverlayWidget = Overlay;
	Overlay->OnSequenceFinished.AddUObject(this, &UGYUIManagerSubsystem::HandleClockOverlayFinished);
	Overlay->PlayResetSequence(HoldDuration);

	if (UGYSoundManager* SoundManager = UGYSoundManager::Get(this))
	{
		SoundManager->PlaySound2D(GYGameplayTags::Sound_World_Clock);
	}
}

void UGYUIManagerSubsystem::StopClockOverlay()
{
	if (ActiveClockOverlayWidget.IsValid())
	{
		ActiveClockOverlayWidget->OnSequenceFinished.RemoveAll(this);
		PopWidget(ActiveClockOverlayWidget.Get());
		ActiveClockOverlayWidget = nullptr;
	}
}

void UGYUIManagerSubsystem::RequestStopClockOverlay()
{
	if (ActiveClockOverlayWidget.IsValid())
	{
		ActiveClockOverlayWidget->RequestFadeOut();
	}
}

void UGYUIManagerSubsystem::HandleClockOverlayFinished()
{
	StopClockOverlay();
}

void UGYUIManagerSubsystem::HandleEndingWaiting(FGameplayTag, const FGYInteractionWaitingMessage& Msg)
{
	APlayerController* LocalPC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!LocalPC) return;

	APlayerState* LocalPS = LocalPC->PlayerState;
	const bool bIsMe = (LocalPS && LocalPS == Msg.ChangedPlayer.Get());

	if (bIsMe)
	{
		if (Msg.bAdded)
		{
			if (!ActiveWaitingWidget.IsValid()) // 상호작용 시 Waiting 위젯 처리
			{
				const UGYUISettings* Settings = GetDefault<UGYUISettings>();
				UClass* WidgetClass = Settings ? Settings->InteractionWaitingWidgetClass.LoadSynchronous() : nullptr;
				if (WidgetClass)
				{
					UCommonActivatableWidget* W = PushSystemWidget(GYUILayerTags::UI_Layer_Menu, WidgetClass);
					ActiveWaitingWidget = Cast<UGYInteractionWaitingWidget>(W);
				}
			}
			if (ActiveWaitingWidget.IsValid())
			{
				ActiveWaitingWidget->SetCount(Msg.CurrentCount, Msg.RequiredCount);
			}
		}
		else
		{
			if (ActiveWaitingWidget.IsValid()) // 상호작용 취소 처리
			{
				PopWidget(ActiveWaitingWidget.Get());
				ActiveWaitingWidget = nullptr;
			}
		}
	}
	else if (ActiveWaitingWidget.IsValid())
	{
		ActiveWaitingWidget->SetCount(Msg.CurrentCount, Msg.RequiredCount); // 다른 사람 상호작용 - 카운트 변경
	}
}

void UGYUIManagerSubsystem::HandleEndingStarted(FGameplayTag, const FGYEndingStartedMessage&)
{
	if (ActiveWaitingWidget.IsValid())
	{
		PopWidget(ActiveWaitingWidget.Get());
		ActiveWaitingWidget = nullptr;
	}
}

void UGYUIManagerSubsystem::HandleEndingCinematicFinished(FGameplayTag, const FGYEndingCinematicFinishedMessage& Msg)
{
	if (Msg.bShowCredits)
	{
		StartEndingNarrative();
	}
}

void UGYUIManagerSubsystem::StartEndingNarrative()
{
	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	UClass* WidgetClass = Settings ? Settings->EndingNarrativeWidgetClass.LoadSynchronous() : nullptr;
	if (!WidgetClass)
	{
		StartEndingCredits();
		return;
	}

	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->ClearAllLayers();
	}

	UCommonActivatableWidget* Widget = PushWidgetToLayer(GYUILayerTags::UI_Layer_Menu, WidgetClass);
	ActiveNarrativeWidget = Widget;
}

void UGYUIManagerSubsystem::HandleEndingNarrativeFinished(FGameplayTag, const FGYEndingNarrativeFinishedMessage&)
{
	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	UClass* WidgetClass = Settings ? Settings->EndingCreditsWidgetClass.LoadSynchronous() : nullptr;
	if (!WidgetClass)
	{
		if (ActiveNarrativeWidget.IsValid())
		{
			PopWidget(ActiveNarrativeWidget.Get());
			ActiveNarrativeWidget = nullptr;
		}
		return;
	}

	UCommonActivatableWidget* Credits = PushWidgetToLayer(GYUILayerTags::UI_Layer_Modal, WidgetClass);
	ActiveCreditsWidget = Credits;

	UWorld* World = GetWorld();
	if (World && Credits)
	{
		World->GetTimerManager().SetTimer(NarrativeCleanupTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (ActiveNarrativeWidget.IsValid())
				{
					PopWidget(ActiveNarrativeWidget.Get());
					ActiveNarrativeWidget = nullptr;
				}
			}),
			EndingCrossfadeHold, /*bLoop=*/ false);
	}
	else if (ActiveNarrativeWidget.IsValid())
	{
		PopWidget(ActiveNarrativeWidget.Get());
		ActiveNarrativeWidget = nullptr;
	}
}

void UGYUIManagerSubsystem::StartEndingCredits()
{
	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	if (!Settings) return;

	UClass* WidgetClass = Settings->EndingCreditsWidgetClass.LoadSynchronous();
	if (!WidgetClass) return;

	if (PrimaryGameLayout) // 다른 위젯 다 삭제
	{
		PrimaryGameLayout->ClearAllLayers();
	}

	UCommonActivatableWidget* Widget = PushSystemWidget(GYUILayerTags::UI_Layer_Menu, WidgetClass);
	ActiveCreditsWidget = Widget;
}

void UGYUIManagerSubsystem::HandleEndingCreditsFinished(FGameplayTag, const FGYEndingCreditsFinishedMessage& /*Msg*/)
{
	if (ActiveCreditsWidget.IsValid()) // 크레딧 위젯 제커
	{
		PopWidget(ActiveCreditsWidget.Get());
		ActiveCreditsWidget = nullptr;
	}
	TravelToMainMenu();
}

void UGYUIManagerSubsystem::TravelToMainMenu() const
{
	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	if (!Settings) return;

	const FSoftObjectPath& MenuMap = Settings->MainMenuMap;
	if (!MenuMap.IsValid()) return;

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC) return;

	PC->ClientTravel(MenuMap.GetLongPackageName(), ETravelType::TRAVEL_Absolute); // 클라 이동 - 추후 메인화면으로 설정
}

void UGYUIManagerSubsystem::HandleToggleSettings(FGameplayTag, const FGYToggleSettingsMessage&)
{
	HandleUIBack(); // ESC 키 - 뒤로가기 처리
}

void UGYUIManagerSubsystem::HandleUIBack()
{
	// ESC가 두 경로로 들어와도 한 프레임에 한 번만 처리
	if (GFrameCounter == LastUIBackFrame) return;
	LastUIBackFrame = GFrameCounter;

	// 시스템 UI 중엔 무시
	if (IsSystemUIActive()) return;

	// 열려 있는 메뉴가 있으면 전부 닫고 끝
	if (ActiveMenuWidgets.Num() > 0)
	{
		CloseAllMenus();
		return;
	}

	// 아무것도 없을 때만 설정창 토글
	const UGYUISettings* UISettings = GetDefault<UGYUISettings>();
	if (!UISettings) return;

	TSubclassOf<UCommonActivatableWidget> Class = UISettings->SettingsWidgetClass.LoadSynchronous();
	if (!Class) return;

	ToggleWidgetInLayer(GYUILayerTags::UI_Layer_Menu, Class);
}

void UGYUIManagerSubsystem::PushUIInteractionBlock(FGameplayTag Reason)
{
	if (!Reason.IsValid()) return;

	ActiveUIBlockReasons.Add(Reason);
	CloseAllMenus();
	RefreshGameplayInputBlock();
}

void UGYUIManagerSubsystem::PopUIInteractionBlock(FGameplayTag Reason)
{
	ActiveUIBlockReasons.Remove(Reason);
	RefreshGameplayInputBlock();
}

void UGYUIManagerSubsystem::HandleEnterCinematic(FGameplayTag, const FGYCinematicMessage& Msg)
{
	// 시네마틱/엔딩 네러티브 동안 위젯 열기 잠금
	if (Msg.bIsPlaying)
	{
		PushUIInteractionBlock(GYGameplayTags::Message_Cinematic_State);
	}
	else
	{
		PopUIInteractionBlock(GYGameplayTags::Message_Cinematic_State);
	}

	if (!PrimaryGameLayout) return;

	UCommonActivatableWidgetContainerBase* Layer = PrimaryGameLayout->GetLayerWidget(GYUILayerTags::UI_Layer_Game);
	if (!Layer) return;

	Layer->SetVisibility(Msg.bIsPlaying ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UGYUIManagerSubsystem::HandleRevivalTagChanged(const FGameplayTag, int32)
{
	UpdateRevivalWidget();
}

void UGYUIManagerSubsystem::UpdateRevivalWidget()
{
	const bool bInRevival = bReviveHoldActive ||
		(BoundASC.IsValid() &&
		 (BoundASC->GetTagCount(GYStateTags::State_Action_Reviving) > 0 ||
		  BoundASC->GetTagCount(GYStateTags::State_Life_BeingRevived) > 0));

	if (bInRevival)
	{
		if (!ActiveRevivalWidget.IsValid())
		{
			ULocalPlayer* LP = GetLocalPlayer();
			APlayerController* PC = LP ? LP->GetPlayerController(GetWorld()) : nullptr;
			const UGYUISettings* Settings = GetDefault<UGYUISettings>();
			UClass* WidgetClass = Settings ? Settings->RevivalWidgetClass.LoadSynchronous() : nullptr;
			if (PC && WidgetClass)
			{
				CloseAllMenus();
				UCommonActivatableWidget* W = CreateWidget<UCommonActivatableWidget>(PC, WidgetClass);
				if (W)
				{
					W->SetVisibility(ESlateVisibility::HitTestInvisible);
					W->AddToViewport(1100);
					ActiveRevivalWidget = W;
				}
			}
		}
	}
	else if (ActiveRevivalWidget.IsValid())
	{
		ActiveRevivalWidget->RemoveFromParent();
		ActiveRevivalWidget = nullptr;
	}
}

void UGYUIManagerSubsystem::HandleReviveHold(FGameplayTag, const FGYReviveHoldMessage& Msg)
{
	bReviveHoldActive = Msg.bHeld;

	UWorld* World = GetWorld();
	if (Msg.bHeld && World)
	{
		GiveUpHoldStartTime = World->GetTimeSeconds();
		GiveUpHoldDuration = Msg.Duration;
		World->GetTimerManager().SetTimer(
			GiveUpProgressTimerHandle, this, &UGYUIManagerSubsystem::TickGiveUpProgress, 0.05f, true);
		TickGiveUpProgress();
	}
	else
	{
		StopGiveUpProgress();
	}

	UpdateRevivalWidget();
}

void UGYUIManagerSubsystem::TickGiveUpProgress()
{
	UWorld* World = GetWorld();
	if (!World || GiveUpHoldDuration <= 0.f) return;

	const float Elapsed = World->GetTimeSeconds() - GiveUpHoldStartTime;

	FGYRevivalProgressMessage Msg;
	Msg.CurrentValue = FMath::Clamp(Elapsed, 0.f, GiveUpHoldDuration);
	Msg.MaxValue = GiveUpHoldDuration;
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(
		GYGameplayTags::Message_Player_RevivalProgress, Msg);
}

void UGYUIManagerSubsystem::StopGiveUpProgress()
{
	bReviveHoldActive = false;
	GiveUpHoldDuration = 0.f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GiveUpProgressTimerHandle);
	}
}
