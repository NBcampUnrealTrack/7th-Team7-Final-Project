// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYEnchantWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Core/GameplayTags/CurrencyTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Currency/CurrencyComponent.h"
#include "Enchant/EnchantCostRow.h"
#include "Enchant/GYEnchantSettings.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Player/GYPlayerState.h"
#include "Widget/Interact/GYEnchantSlotWidget.h"
#include "Widget/Inventory/GYInventoryScreenWidget.h"
#include "Widget/ItemInfo/GYItemInfoWidget.h"

void UGYEnchantWidget::OnCurrencyChanged(FGameplayTag GameplayTag, int32 Amount)
{
	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();

	int32 MaxAmount = 100;
	UDataTable* CostTable = IsValid(Settings) ? Settings->EnchantCostTable.LoadSynchronous() : nullptr;
	if (IsValid(CostTable))
	{
		const FEnchantCostRow* CostRow = CostTable->FindRow<FEnchantCostRow>(Settings->DefaultCostRowName, TEXT("EnchantService::TryEnchant"));
		MaxAmount = CostRow->Amount;
	}
	ProgressBar->SetPercent(static_cast<float>(Amount)/MaxAmount);
	if (Text_TimeShard)
	{
		Text_TimeShard->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Amount, MaxAmount)));
	}
}

void UGYEnchantWidget::NativeConstruct()
{
	Super::NativeConstruct();


	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UCurrencyComponent* CurrencyComponent =  IsValid(PS) ? PS->GetCurrencyComponent() : nullptr;

	int32 CurrentAmount = CurrencyComponent->GetAmount(GYGameplayTags::Currency_TimeShard);
	OnCurrencyChangedHandle = CurrencyComponent->OnCurrencyChanged.AddUObject(this, &UGYEnchantWidget::OnCurrencyChanged);

	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();

	int32 MaxAmount = 100;
	UDataTable* CostTable = IsValid(Settings) ? Settings->EnchantCostTable.LoadSynchronous() : nullptr;
	if (IsValid(CostTable))
	{
		const FEnchantCostRow* CostRow = CostTable->FindRow<FEnchantCostRow>(Settings->DefaultCostRowName, TEXT("EnchantService::TryEnchant"));
		MaxAmount = CostRow->Amount;
	}

	ProgressBar->SetPercent(static_cast<float>(CurrentAmount)/MaxAmount);
	if (Text_TimeShard)
	{
		Text_TimeShard->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentAmount, MaxAmount)));
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UGYEnchantWidget::OnCloseButtonClicked);
	}
	if (ExecuteButton)
	{
		ExecuteButton->OnClicked.AddDynamic(this, &UGYEnchantWidget::OnExecuteButtonClicked);
	}

	// 임베드된 인벤토리의 아이템 좌클릭 → 인첸트 대상 지정
	InventoryScreen = Cast<UGYInventoryScreenWidget>(GetWidgetFromName(TEXT("WBP_InventoryScreen")));
	if (InventoryScreen)
	{
		InventoryScreen->OnItemClicked.AddDynamic(this, &UGYEnchantWidget::HandleInventoryItemClicked);
	}
}

void UGYEnchantWidget::NativeDestruct()
{
	Super::NativeDestruct();
	CloseButton->OnClicked.RemoveDynamic(this, &UGYEnchantWidget::OnCloseButtonClicked);
	ExecuteButton->OnClicked.RemoveDynamic(this, &UGYEnchantWidget::OnExecuteButtonClicked);

	if (InventoryScreen)
	{
		InventoryScreen->OnItemClicked.RemoveDynamic(this, &UGYEnchantWidget::HandleInventoryItemClicked);
	}


	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UCurrencyComponent* CurrencyComponent =  IsValid(PS) ? PS->GetCurrencyComponent() : nullptr;
	CurrencyComponent->OnCurrencyChanged.Remove(OnCurrencyChangedHandle);
}

void UGYEnchantWidget::OnCloseButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Enchant_Exit, FGameplayEventData());
}

void UGYEnchantWidget::HandleInventoryItemClicked(FGuid InstanceId)
{
	SetTarget(InstanceId);
}

void UGYEnchantWidget::SetTarget(const FGuid& InstanceId)
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UInventoryComponent* Inventory = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	const FInventoryEntry* Entry = IsValid(Inventory) ? Inventory->FindEntry(InstanceId) : nullptr;
	if (Entry == nullptr) return;

	if (EnchantSlotWidget)
	{
		EnchantSlotWidget->SetEntry(*Entry);
	}

	if (TargetInfo)
	{
		FGYItemViewData View;
		View.Definition = Entry->Definition;
		View.InstanceId = Entry->InstanceId;
		View.GradeTag = Entry->GradeTag;
		View.Level = Entry->Level;
		View.Count = Entry->StackCount;
		View.StatDeviation = Entry->StatDeviation;
		View.RolledOptions = Entry->RolledOptions;
		TargetInfo->ShowItem(View);
	}
}

void UGYEnchantWidget::OnExecuteButtonClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UInventoryComponent* InventoryComponent =  IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	if (InventoryComponent)
	{
		InventoryComponent->Server_RequestEnchant(EnchantSlotWidget->GetItemInstanceId());
	}
}
