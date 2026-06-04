// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYTimeRiftWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Components/Button.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"

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
	if (RerollButton != nullptr)
	{
		RerollButton->OnClicked.AddDynamic(this, &UGYTimeRiftWidget::OnRerollButtonClicked);
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
	RerollButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnRerollButtonClicked);
	SkillTreeButton->OnClicked.RemoveDynamic(this, &UGYTimeRiftWidget::OnSkillTreeButtonClicked);
}

void UGYTimeRiftWidget::OnExitButtonClicked()
{

	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Exit, FGameplayEventData());

}

void UGYTimeRiftWidget::OnRestButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
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

void UGYTimeRiftWidget::OnRerollButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Reroll, FGameplayEventData());
}

void UGYTimeRiftWidget::OnSkillTreeButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_SkillTree, FGameplayEventData());
}
