#include "SkillTreeEditor/SkillTreeAssetEditor.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTreeEditor/SkillTreeGraph.h"
#include "SkillTreeEditor/SkillTreeGraphSchema.h"
#include "SkillTreeEditor/EdGraphNode_SkillNode.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"

const FName FSkillTreeAssetEditor::GraphTabId = "SkillTreeAssetEditor_Graph";
const FName FSkillTreeAssetEditor::DetailsTabId = "SkillTreeAssetEditor_Details";

void FSkillTreeAssetEditor::InitEditor(
	EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& Host,
	USkillTreeDataAsset* InAsset)
{
	ensureMsgf(InAsset, TEXT("InAsset is nullptr"));

	SkillTreeAsset = InAsset;

	EditorGraph = NewObject<USkillTreeGraph>(InAsset, NAME_None, RF_Transactional);
	EditorGraph->Schema = USkillTreeGraphSchema::StaticClass();

	RebuildGraph();
	CreateGraphEditor();
	CreateDetailsView();

	const TSharedRef<FTabManager::FLayout> Layout =
		FTabManager::NewLayout("SkillTreeAssetEditor")
		->AddArea(FTabManager::NewPrimaryArea()
		          ->SetOrientation(Orient_Horizontal)
		          ->Split(FTabManager::NewStack()
		                  ->SetSizeCoefficient(0.75f)
		                  ->AddTab(GraphTabId, ETabState::OpenedTab)
		                  ->SetHideTabWell(true))
		          ->Split(FTabManager::NewStack()
		                  ->SetSizeCoefficient(0.25f)
		                  ->AddTab(DetailsTabId, ETabState::OpenedTab)
		                  ->SetHideTabWell(true)));

	InitAssetEditor(Mode, Host, "SkillTreeAssetEditor",
	                Layout, true, true, InAsset);
}

void FSkillTreeAssetEditor::RegisterTabSpawners(
	const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(
		          GraphTabId,
		          FOnSpawnTab::CreateSP(this, &FSkillTreeAssetEditor::SpawnGraphTab))
	          .SetDisplayName(NSLOCTEXT("SkillTree", "GraphTab", "Graph"))
	          .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(
		          DetailsTabId,
		          FOnSpawnTab::CreateSP(this, &FSkillTreeAssetEditor::SpawnDetailsTab))
	          .SetDisplayName(NSLOCTEXT("SkillTree", "DetailsTab", "Details"))
	          .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FSkillTreeAssetEditor::UnregisterTabSpawners(
	const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

TSharedRef<SDockTab> FSkillTreeAssetEditor::SpawnGraphTab(const FSpawnTabArgs&)
{
	return SNew(SDockTab).TabRole(ETabRole::PanelTab)
		[
			GraphEditor.IsValid() ? GraphEditor.ToSharedRef() : SNullWidget::NullWidget
		];
}

TSharedRef<SDockTab> FSkillTreeAssetEditor::SpawnDetailsTab(const FSpawnTabArgs&)
{
	return SNew(SDockTab).TabRole(ETabRole::PanelTab)
		[
			DetailsView.IsValid() ? DetailsView.ToSharedRef() : SNullWidget::NullWidget
		];
}

void FSkillTreeAssetEditor::CreateGraphEditor()
{
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(
		this, &FSkillTreeAssetEditor::OnSelectionChanged);
	Events.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(
		this, &FSkillTreeAssetEditor::OnNodeDoubleClicked);

	GraphEditor = SNew(SGraphEditor)
		.IsEditable(true)
		.GraphToEdit(EditorGraph)
		.GraphEvents(Events);
}

void FSkillTreeAssetEditor::CreateDetailsView()
{
	FPropertyEditorModule& PropModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	DetailsView = PropModule.CreateDetailView(Args);
	DetailsView->SetObject(SkillTreeAsset);
}

void FSkillTreeAssetEditor::RebuildGraph()
{
	EditorGraph->RebuildGraphFromAsset(SkillTreeAsset);
}

void FSkillTreeAssetEditor::CompileAsset()
{
	EditorGraph->CompileAsset(SkillTreeAsset);
}

void FSkillTreeAssetEditor::SaveAsset_Execute()
{
	CompileAsset();
	FAssetEditorToolkit::SaveAsset_Execute();
}

void FSkillTreeAssetEditor::OnSelectionChanged(const TSet<UObject*>& Selected)
{
	TArray<UObject*> Objects;
	for (UObject* Obj : Selected)
	{
		if (UEdGraphNode_SkillNode* N = Cast<UEdGraphNode_SkillNode>(Obj))
		{
			if (N->SkillAsset)
			{
				Objects.Add(N->SkillAsset);
			}
		}
	}
	DetailsView->SetObjects(Objects.Num() > 0 ? Objects : TArray<UObject*>{SkillTreeAsset});
}

void FSkillTreeAssetEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	if (UEdGraphNode_SkillNode* N = Cast<UEdGraphNode_SkillNode>(Node))
	{
		if (N->SkillAsset)
		{
			GEditor->SyncBrowserToObjects(TArray<UObject*>{N->SkillAsset});
		}
	}
}

void FSkillTreeAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
                                             FProperty* PropertyThatChanged)
{
	if (GraphEditor.IsValid())
	{
		GraphEditor->NotifyGraphChanged();
	}
}
