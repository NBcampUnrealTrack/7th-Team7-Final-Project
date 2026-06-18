#include "Widget/Inventory/GYPlayerStatsWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerState.h"
#include "Widget/Inventory/GYStatRowWidget.h"

void UGYPlayerStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildRows();
	if (Text_PlayerName)
	{
		APlayerController* PC = GetOwningPlayer();
		APlayerState* PS = IsValid(PC) ? PC->PlayerState : nullptr;
		if (IsValid(PS))
		{
			Text_PlayerName->SetText(FText::FromString(PS->GetPlayerName()));
		}
	}
	BindToOwningASC();
	RefreshAllRows();
}

void UGYPlayerStatsWidget::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

void UGYPlayerStatsWidget::BuildRows()
{
	SpawnedRows.Reset();
	AttributeToRowIndices.Reset();

	for (int32 Index = 0; Index < StatRows.Num(); ++Index)
	{
		AttributeToRowIndices.FindOrAdd(StatRows[Index].Attribute).Add(Index);
	}

	if (StatRowContainer == nullptr || StatRowWidgetClass == nullptr) return;
	StatRowContainer->ClearChildren();

	for (const FGYPlayerStatRowDefinition& Definition : StatRows)
	{
		UGYStatRowWidget* RowWidget = WidgetTree->ConstructWidget<UGYStatRowWidget>(StatRowWidgetClass);
		if (!IsValid(RowWidget)) continue;

		RowWidget->Label = Definition.Label;
		RowWidget->bIntegerDisplay = Definition.bIntegerDisplay;
		RowWidget->bPercentDisplay = Definition.bPercentDisplay;

		StatRowContainer->AddChild(RowWidget);
		SpawnedRows.Add(RowWidget);
	}
}

void UGYPlayerStatsWidget::BindToOwningASC()
{
	UnbindFromASC();

	APlayerController* PC = GetOwningPlayer();
	APlayerState* PS = IsValid(PC) ? PC->PlayerState : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
	if (!IsValid(ASC)) return;

	BoundASC = ASC;

	TSet<FGameplayAttribute> Seen;
	for (const FGYPlayerStatRowDefinition& Definition : StatRows)
	{
		if (!Definition.Attribute.IsValid()) continue;
		if (Seen.Contains(Definition.Attribute)) continue;
		Seen.Add(Definition.Attribute);

		FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Definition.Attribute).AddUObject(
			this, &UGYPlayerStatsWidget::HandleAttributeChanged);

		Bindings.Add({Definition.Attribute, Handle});
	}
}

void UGYPlayerStatsWidget::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const FBoundAttribute& Bound : Bindings)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Bound.Attribute).Remove(Bound.Handle);
		}
	}
	Bindings.Reset();
	BoundASC = nullptr;
}

void UGYPlayerStatsWidget::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (const TArray<int32>* Indices = AttributeToRowIndices.Find(Data.Attribute))
	{
		for (int32 Index : *Indices)
		{
			RefreshRow(Index);
		}
	}
}

void UGYPlayerStatsWidget::RefreshAllRows()
{
	for (int32 Index = 0; Index < StatRows.Num(); ++Index)
	{
		RefreshRow(Index);
	}
}

void UGYPlayerStatsWidget::RefreshRow(int32 RowIndex)
{
	if (!StatRows.IsValidIndex(RowIndex)) return;
	if (!SpawnedRows.IsValidIndex(RowIndex)) return;

	UGYStatRowWidget* Row = SpawnedRows[RowIndex];
	if (!IsValid(Row)) return;

	const FGYPlayerStatRowDefinition& Definition = StatRows[RowIndex];
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!IsValid(ASC) || !Definition.Attribute.IsValid())
	{
		Row->SetValues(0.f, 0.f);
		return;
	}
	const float Base = ASC->GetNumericAttributeBase(Definition.Attribute);
	const float Current = ASC->GetNumericAttribute(Definition.Attribute);
	Row->SetValues(Base, Current - Base);
}
