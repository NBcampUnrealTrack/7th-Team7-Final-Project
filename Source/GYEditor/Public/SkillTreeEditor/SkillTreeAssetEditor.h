#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Misc/NotifyHook.h"

class USkillTreeGraph;
class USkillTreeDataAsset;
class SGraphEditor;
class IDetailsView;

class FSkillTreeAssetEditor : public FAssetEditorToolkit, public FNotifyHook
{
public:
    void InitEditor(EToolkitMode::Type Mode,
        const TSharedPtr<IToolkitHost>& Host,
        USkillTreeDataAsset* InAsset);

    virtual FName        GetToolkitFName()             const override { return "SkillTreeAssetEditor"; }
    virtual FText        GetBaseToolkitName()           const override { return INVTEXT("Skill Tree Editor"); }
    virtual FString      GetWorldCentricTabPrefix()     const override { return "SkillTree "; }
    virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0, 0.8f, 0.4f); }

    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>&) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>&) override;
    virtual void SaveAsset_Execute() override;
    virtual void NotifyPostChange(const FPropertyChangedEvent&, FProperty*) override;

private:
    void CreateGraphEditor();
    void CreateDetailsView();
    void RebuildGraph();
    void CompileAsset();

    void OnSelectionChanged(const TSet<UObject*>& Selected);
    void OnNodeDoubleClicked(UEdGraphNode* Node);

    TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

    TObjectPtr<USkillTreeDataAsset> SkillTreeAsset;
	TObjectPtr<USkillTreeGraph>  EditorGraph;
    TSharedPtr<SGraphEditor>    GraphEditor;
    TSharedPtr<IDetailsView>    DetailsView;

    static const FName GraphTabId;
    static const FName DetailsTabId;
};
