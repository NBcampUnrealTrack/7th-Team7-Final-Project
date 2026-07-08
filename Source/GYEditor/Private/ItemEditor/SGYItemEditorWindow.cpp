#include "ItemEditor/SGYItemEditorWindow.h"

#include "ItemEditor/GYItemEditorController.h"
#include "ItemEditor/SGYItemBrowserPanel.h"
#include "ItemEditor/SGYItemDetailPanel.h"

#include "Widgets/Layout/SSplitter.h"

void SGYItemEditorWindow::Construct(const FArguments& InArgs)
{
	Controller = MakeShared<FGYItemEditorController>();
	Controller->Initialize();

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SAssignNew(BrowserPanel, SGYItemBrowserPanel, Controller.ToSharedRef())
			.OnItemSelected(this, &SGYItemEditorWindow::HandleItemSelected)
		]
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SAssignNew(DetailPanel, SGYItemDetailPanel, Controller.ToSharedRef())
		]
	];

	Controller->OnDataChanged.AddSP(this, &SGYItemEditorWindow::HandleDataChanged);
}

SGYItemEditorWindow::~SGYItemEditorWindow()
{
	if (Controller.IsValid())
	{
		Controller->Shutdown();
	}
}

void SGYItemEditorWindow::HandleDataChanged()
{
	BrowserPanel->RefreshList();
	DetailPanel->RefreshAll();
}

void SGYItemEditorWindow::HandleItemSelected(UItemDefinition* Item)
{
	DetailPanel->SetItem(Item);
}
