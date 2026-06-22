#include "Core/GYUIManagerSubsystem.h"
#include "GYUI/Public/Core/GYPrimaryGameLayout.h"
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
#include "Core/GameplayTags/StateTags.h"
#include "UI/GYUIMessages.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/GameStateBase.h"
#include "Character/GYPawnData.h"
#include "Character/GYPlayerActionConfig.h"
#include "Widget/EndingCredits/GYEndingCreditsWidget.h"
#include "Widget/Interaction/GYInteractionWaitingWidget.h"
#include "Widget/WorldReset/GYWorldResetWidget.h"

void UGYUIManagerSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RosterSyncHandle);
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MSG = GI->GetSubsystem<UGameplayMessageSubsystem>())
			{
				MSG->UnregisterListener(RegionEnterListenerHandle);
				MSG->UnregisterListener(RegionExitListenerHandle);
				MSG->UnregisterListener(EndingStartedHandle);
				MSG->UnregisterListener(EndingWaitingHandle);
				MSG->UnregisterListener(EndingCinematicFinishedHandle);
				MSG->UnregisterListener(EndingCreditsFinishedHandle);
				MSG->UnregisterListener(WorldResetListenerHandle);
				MSG->UnregisterListener(ToggleSettingsListenerHandle);
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
			PushWidgetToLayer(GYUILayerTags::UI_Layer_Game, HUDClass);
		}
		if (UClass* ReviveClass = Settings->RevivalWidgetClass.LoadSynchronous())
		{
			RegisterTagDrivenWidget(GYStateTags::State_Life_Dead,GYUILayerTags::UI_Layer_Modal, ReviveClass);
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
		if (RegionEnterListenerHandle.IsValid())
		{
			MSG.UnregisterListener(RegionEnterListenerHandle);
		}
		if (RegionExitListenerHandle.IsValid())
		{
			MSG.UnregisterListener(RegionExitListenerHandle);
		}
		if (EndingStartedHandle.IsValid())
		{
			MSG.UnregisterListener(EndingStartedHandle);
		}
		if (EndingWaitingHandle.IsValid())
		{
			MSG.UnregisterListener(EndingWaitingHandle);
		}
		if (EndingCinematicFinishedHandle.IsValid())
		{
			MSG.UnregisterListener(EndingCinematicFinishedHandle);
		}
		if (EndingCreditsFinishedHandle.IsValid())
		{
			MSG.UnregisterListener(EndingCreditsFinishedHandle);
		}
		if (WorldResetListenerHandle.IsValid())
		{
			MSG.UnregisterListener(WorldResetListenerHandle);
		}
		if (ToggleSettingsListenerHandle.IsValid())
		{
			MSG.UnregisterListener(ToggleSettingsListenerHandle);
		}

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
		EndingCreditsFinishedHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_CreditsFinished, this, &UGYUIManagerSubsystem::HandleEndingCreditsFinished);
		WorldResetListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_World_Reset, this, &UGYUIManagerSubsystem::HandleWorldReset);
		ToggleSettingsListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_UI_ToggleSettings, this, &UGYUIManagerSubsystem::HandleToggleSettings);
	}
}

void UGYUIManagerSubsystem::HandlePlayerStateInitialized(AGYPlayerController* PC)
{
	if (!PC) return;
	AGYPlayerState* PS = PC->GetPlayerState<AGYPlayerState>();
	if (!PS) return;
	if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
	{
		BindASC(ASC);
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

	// 부활 진행도 브로드캐스트 트리거
	DeathTagHandle = InASC->RegisterGameplayTagEvent(GYStateTags::State_Life_Dead, EGameplayTagEventType::NewOrRemoved)
	.AddWeakLambda(this, [this](const FGameplayTag, int32 NewCount)
		{
		if (NewCount > 0) StartRevivalBroadcast(); // 사망 시 부활 브로드캐스트
		else StopRevivalBroadcast(); // 부활 시 브로드캐스트
		});

	for (const auto& Pair : TagWidgetMap)
	{
		const int32 Count = InASC->GetTagCount(Pair.Key);
		if (Count > 0) OnTagChanged(Pair.Key, Count);
	}

	if (InASC->GetTagCount(GYStateTags::State_Life_Dead) > 0)
	{
		StartRevivalBroadcast();
	}
}

void UGYUIManagerSubsystem::RegisterTagDrivenWidget(
	FGameplayTag StateTag,
	FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
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
			UCommonActivatableWidget* Widget = PushWidgetToLayer(Entry->LayerTag, Entry->WidgetClass);
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
	if (PrimaryGameLayout) return; // 중복 생성 방지
	if (!LayoutClass) return;

	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;

	APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
	if (!PC) return;

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
}

UCommonActivatableWidget* UGYUIManagerSubsystem::PushWidgetToLayer(FGameplayTag LayerTag,
                                                                   TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryGameLayout) return nullptr;

	// 레이아웃의 내부 로직으로 위젯 Push
	return PrimaryGameLayout->PushWidgetToLayer(LayerTag, WidgetClass);
}

void UGYUIManagerSubsystem::PopWidget(UCommonActivatableWidget* Widget)
{
	if (PrimaryGameLayout && Widget)
	{
		PrimaryGameLayout->RemoveWidgetFromLayer(Widget);
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
		StopRevivalBroadcast();
		UnregisterStatBroadcast();
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
    APawn* LocalPawn = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld())->GetPawn() : nullptr;
    if (!LocalPawn || Msg.Pawn.Get() != LocalPawn) return;

	ActiveRegionId = Msg.RegionId;
	if (IsValid(Msg.BossActor))
	{
		FGYBossStateMessage State;
		State.bVisible = true;
		State.TargetBoss = Msg.BossActor;

		UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Boss_State, State);
	}
}

void UGYUIManagerSubsystem::HandleRegionExited(FGameplayTag Tag, const FGYRegionExitedMessage& Msg)
{
    APawn* LocalPawn = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld())->GetPawn() : nullptr;
    if (Msg.Pawn.Get() != LocalPawn) return;

	if (Msg.RegionId == ActiveRegionId)
	{
		ActiveRegionId = FGameplayTag(); // 지역 정보 초기화

		// UI 끄기
		FGYBossStateMessage State;
		State.bVisible = false;
		State.TargetBoss = nullptr;
		UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Boss_State, State);
	}
}

void UGYUIManagerSubsystem::StartRevivalBroadcast()
{
	if (bRevivalActive) return;

	RevivalDuration = 5.f;
	if (APlayerState* PS = GetLocalPlayerState())
	{
		if (const AGYPlayerState* GYPS = Cast<AGYPlayerState>(PS))
		{
			if (const UGYPawnData* Data = GYPS->GetPawnData())
			{
				if (Data->ActionConfig)
				{
					RevivalDuration = FMath::Max(Data->ActionConfig->RespawnDelay, 0.01f);
				}
			}
		}
	}
	RevivalElapsed = 0.f;
	bRevivalActive = true;

	BroadcastRevivalProgress(0.f, RevivalDuration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RevivalTickHandle,FTimerDelegate::CreateUObject(
			this, &UGYUIManagerSubsystem::TickRevivalBroadcast),RevivalBroadcastInterval, true);
	}
}

void UGYUIManagerSubsystem::StopRevivalBroadcast()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevivalTickHandle);
	}
	bRevivalActive = false;
	RevivalElapsed = 0.f;
	RevivalDuration = 0.f;
}

void UGYUIManagerSubsystem::TickRevivalBroadcast()
{
	if (!bRevivalActive) return;

	RevivalElapsed = FMath::Min(RevivalElapsed + RevivalBroadcastInterval, RevivalDuration);
	BroadcastRevivalProgress(RevivalElapsed, RevivalDuration);
	if (RevivalElapsed >= RevivalDuration)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RevivalTickHandle);
		}
		bRevivalActive = false;
	}
}

void UGYUIManagerSubsystem::BroadcastRevivalProgress(float Current, float Max) const
{
	if (!GetWorld()) return;
	FGYRevivalProgressMessage Msg;
	Msg.CurrentValue = Current;
	Msg.MaxValue = Max;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(
		GYGameplayTags::Message_Player_RevivalProgress, Msg);
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
					UCommonActivatableWidget* W = PushWidgetToLayer(GYUILayerTags::UI_Layer_Menu, WidgetClass);
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
		StartEndingCredits();
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

	UCommonActivatableWidget* Widget = PushWidgetToLayer(GYUILayerTags::UI_Layer_Menu, WidgetClass);
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

void UGYUIManagerSubsystem::HandleWorldReset(FGameplayTag, const FGYWorldResetMessage& Msg)
{
	const UGYUISettings* Settings = GetDefault<UGYUISettings>();
	UClass* WidgetClass = Settings ? Settings->WorldResetWidgetClass.LoadSynchronous() : nullptr;
	if (!WidgetClass) return;

	if (ActiveWorldResetWidget.IsValid())
	{
		ActiveWorldResetWidget->PlayResetSequence(Msg.DurationOverride);
		return;
	}

	UCommonActivatableWidget* W = PushWidgetToLayer(GYUILayerTags::UI_Layer_Menu, WidgetClass);
	UGYWorldResetWidget* Reset = Cast<UGYWorldResetWidget>(W);
	if (!Reset) return;

	ActiveWorldResetWidget = Reset;
	Reset->OnSequenceFinished.AddUObject(this, &UGYUIManagerSubsystem::HandleWorldResetFinished);
	Reset->PlayResetSequence(Msg.DurationOverride);
}

void UGYUIManagerSubsystem::HandleWorldResetFinished()
{
	if (ActiveWorldResetWidget.IsValid())
	{
		// 중복 바인딩 방지
		ActiveWorldResetWidget->OnSequenceFinished.RemoveAll(this);
		PopWidget(ActiveWorldResetWidget.Get());
		ActiveWorldResetWidget = nullptr;
	}
}

void UGYUIManagerSubsystem::HandleToggleSettings(FGameplayTag, const FGYToggleSettingsMessage&)
{
	const UGYUISettings* UISettings = GetDefault<UGYUISettings>();
	if (!UISettings) return;

	TSubclassOf<UCommonActivatableWidget> Class = UISettings->SettingsWidgetClass.LoadSynchronous();
	if (!Class) return;

	ToggleWidgetInLayer(GYUILayerTags::UI_Layer_Menu, Class);
}
