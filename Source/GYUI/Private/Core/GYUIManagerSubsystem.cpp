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

void UGYUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGYUIManagerSubsystem::Deinitialize()
{
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
		GYPC->OnPlayerStateInitialized.AddUObject(this, &UGYUIManagerSubsystem::HandlePlayerStateInitialized);
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

void UGYUIManagerSubsystem::BindASC(UAbilitySystemComponent* InASC)
{
	if (!InASC || BoundASC == InASC)
	{
		return;
	}

	if (BoundASC.IsValid())
	{
		for (auto& Pair : TagWidgetMap)
		{
			BoundASC->UnregisterGameplayTagEvent(
				Pair.Value.DelegateHandle,
				Pair.Key,
				EGameplayTagEventType::NewOrRemoved);
		}
	}

	BoundASC = InASC;

	// 등록된 태그 전부 재구독
	for (auto& Pair : TagWidgetMap)
	{
		Pair.Value.DelegateHandle =
			InASC->RegisterGameplayTagEvent(Pair.Key, EGameplayTagEventType::NewOrRemoved)
			     .AddUObject(this, &UGYUIManagerSubsystem::OnTagChanged);
	}
}

void UGYUIManagerSubsystem::RegisterTagDrivenWidget(
	FGameplayTag StateTag,
	FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
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
	if (!Entry)
	{
		return;
	}
	//위젯push
	if (NewCount > 0)
	{
		if (!Entry->ActiveWidget.IsValid())
		{
			UCommonActivatableWidget* Widget = PushWidgetToLayer(Entry->LayerTag, Entry->WidgetClass);
			Entry->ActiveWidget = Widget;

			if (Widget)
			{
				Widget->OnDeactivated().AddLambda([Entry]()
				{
					Entry->ActiveWidget = nullptr;
				});
			}
		}
	}
	else//위젯 pop
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
