// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYTimeRiftWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Components/Button.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UI/GYUIMessages.h"

void UGYTimeRiftWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ExitButton != nullptr)
	{
		ExitButton->OnClicked.AddDynamic(this, &UGYTimeRiftWidget::OnExitButtonClicked);
	}
	if (RestButton != nullptr)
	{
		RestButton->OnClicked.AddDynamic(this, &UGYTimeRiftWidget::OnRestButtonClicked);
	}
	if (AltarButton != nullptr)
	{
		AltarButton->OnClicked.AddDynamic(this, &UGYTimeRiftWidget::OnAltarButtonClicked);
	}
	if (EnchantButton != nullptr)
	{
		EnchantButton->OnClicked.AddDynamic(this, &UGYTimeRiftWidget::OnEnchantButtonClicked);
	}
	if (SkillTreeButton != nullptr)
	{
		SkillTreeButton->OnClicked.AddDynamic(this, &UGYTimeRiftWidget::OnSkillTreeButtonClicked);
	}
}

void UGYTimeRiftWidget::NativeDestruct()
{
	Super::NativeDestruct();
	ExitButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnExitButtonClicked);
	RestButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnRestButtonClicked);
	AltarButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnAltarButtonClicked);
	EnchantButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnEnchantButtonClicked);
	SkillTreeButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnSkillTreeButtonClicked);
}

void UGYTimeRiftWidget::OnExitButtonClicked()
{
	RequestExit();
}

void UGYTimeRiftWidget::OnRestButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	if (UWorld* World = GetWorld())
	{
		FGYClockOverlayMessage Out;
		Out.HoldDuration = RestOverlayDuration;
		Out.Reason = GYGameplayTags::Event_TimeRift_Rest;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_UI_ClockOverlay, Out);
	}

	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Rest, FGameplayEventData());
}

void UGYTimeRiftWidget::OnAltarButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Altar, FGameplayEventData());
}

void UGYTimeRiftWidget::OnEnchantButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Enchant, FGameplayEventData());
}

void UGYTimeRiftWidget::OnSkillTreeButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_SkillTree, FGameplayEventData());
}

FGameplayTag UGYTimeRiftWidget::GetExitEventTag() const
{
	return GYGameplayTags::Event_TimeRift_Exit;
}
