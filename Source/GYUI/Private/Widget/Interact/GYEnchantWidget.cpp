// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYEnchantWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Core/GameplayTags/CurrencyTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Currency/CurrencyComponent.h"
#include "Enchant/EnchantCostRow.h"
#include "Enchant/GYEnchantSettings.h"
#include "Inventory/InventoryComponent.h"
#include "Player/GYPlayerState.h"
#include "Widget/Interact/GYEnchantSlotWidget.h"

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

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UGYEnchantWidget::OnCloseButtonClicked);
	}
	if (ExecuteButton)
	{
		ExecuteButton->OnClicked.AddDynamic(this, &UGYEnchantWidget::OnExecuteButtonClicked);
	}
}

void UGYEnchantWidget::NativeDestruct()
{
	Super::NativeDestruct();
	CloseButton->OnClicked.RemoveDynamic(this, &UGYEnchantWidget::OnCloseButtonClicked);
	ExecuteButton->OnClicked.RemoveDynamic(this, &UGYEnchantWidget::OnExecuteButtonClicked);


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
