#include "ItemEditor/SGYItemDetailPanel.h"

#include "ItemEditor/GYItemEditorController.h"
#include "ItemEditor/SGYItemPoolPanel.h"
#include "ItemEditor/SGYItemStatsPanel.h"
#include "ItemEditor/SGYItemValidationPanel.h"

#include "Items/ItemDefinition.h"

#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/SBoxPanel.h"

void SGYItemDetailPanel::Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController)
{
	Controller = InController;

	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;

	DetailsView = PropertyModule.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SAssignNew(ValidationPanel, SGYItemValidationPanel)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f, 0.f)
		[
			SAssignNew(StatsPanel, SGYItemStatsPanel, InController)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SAssignNew(PoolPanel, SGYItemPoolPanel, InController)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(4.f, 0.f, 4.f, 4.f)
		[
			DetailsView.ToSharedRef()
		]
	];
}

void SGYItemDetailPanel::SetItem(UItemDefinition* Item)
{
	CurrentItem = Item;
	DetailsView->SetObject(Item);
	StatsPanel->SetItem(Item);
	PoolPanel->SetItem(Item);
	RefreshValidation();
}

void SGYItemDetailPanel::RefreshAll()
{
	RefreshValidation();
	StatsPanel->Refresh();
	PoolPanel->Refresh();
}

void SGYItemDetailPanel::RefreshValidation()
{
	UItemDefinition* Item = CurrentItem.Get();
	if (Item == nullptr)
	{
		ValidationPanel->SetReport(nullptr);
		return;
	}

	static const FGYItemValidationReport EmptyReport;
	const FGYItemValidationReport* Report = Controller->FindReport(Item);
	ValidationPanel->SetReport(Report != nullptr ? Report : &EmptyReport);
}
