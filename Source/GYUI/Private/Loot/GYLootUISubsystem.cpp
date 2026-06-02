#include "Loot/GYLootUISubsystem.h"

#include "Core/GYUIManagerSubsystem.h"
#include "Core/GYUISettings.h"
#include "GameplayTags/GYUILayerTags.h"
#include "Widget/Loot/GYLootBoxScreenWidget.h"

#include "CommonActivatableWidget.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Loot/LootBoxActor.h"
#include "UI/GYUIMessages.h"

bool UGYLootUISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	if (IsRunningDedicatedServer()) return false;
	return true;
}

void UGYLootUISubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (!IsValid(NewPlayerController) || !NewPlayerController->IsLocalController()) return;
	if (ListenerHandle.IsValid()) return;

	UWorld* World = GetWorld();
	if (World == nullptr) return;

	ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
		GYGameplayTags::Message_Loot_ShowBox,
		this,
		&UGYLootUISubsystem::HandleShowBox);
}

void UGYLootUISubsystem::Deinitialize()
{
	if (ListenerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayMessageSubsystem::Get(World).UnregisterListener(ListenerHandle);
		}
		ListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::Deinitialize();
}

void UGYLootUISubsystem::HandleShowBox(FGameplayTag, const FGYLootBoxStateMessage& Message)
{
	ALootBoxActor* Box = Cast<ALootBoxActor>(Message.Box.Get());
	if (!IsValid(Box)) return;

	if (ActiveScreen.IsValid())
	{
		// 같은 박스에 다시 상호작용 → 토글로 닫기 (위젯 자체가 ReleaseOccupancy 처리)
		if (ActiveBox.Get() == Box)
		{
			ActiveScreen->DeactivateWidget();
			ActiveScreen = nullptr;
			ActiveBox = nullptr;
			return;
		}

		// 다른 박스 → 박스만 교체
		ActiveScreen->BindToBox(Box);
		ActiveBox = Box;
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UGYUIManagerSubsystem* UIManager = IsValid(LocalPlayer) ? LocalPlayer->GetSubsystem<UGYUIManagerSubsystem>() : nullptr;
	if (!IsValid(UIManager)) return;

	UClass* ScreenClass = GetDefault<UGYUISettings>()->LootBoxScreenClass.LoadSynchronous();
	if (ScreenClass == nullptr) return;

	UCommonActivatableWidget* Pushed = UIManager->PushWidgetToLayer(GYUILayerTags::UI_Layer_Menu, ScreenClass);
	UGYLootBoxScreenWidget* Screen = Cast<UGYLootBoxScreenWidget>(Pushed);
	if (!IsValid(Screen)) return;

	Screen->BindToBox(Box);
	ActiveScreen = Screen;
	ActiveBox = Box;
}
