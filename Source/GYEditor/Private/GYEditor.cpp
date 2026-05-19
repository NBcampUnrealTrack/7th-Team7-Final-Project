#include "GYEditor.h"
#include "Debug/GYDebugMenuManager.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "SkillTreeEditor/SkillTreeNodeFactory.h"
#include "SkillTreeEditor/AssetTypeActions_SkillTree.h"

#define LOCTEXT_NAMESPACE "FGYEditorModule"

void FGYEditorModule::StartupModule()
{
    DebugMenuManager = MakeShared<FGYDebugMenuManager>();
    DebugMenuManager->Initialize();


	SkillNodeFactory = MakeShared<FSkillTreeNodeFactory>();
	SkillPinFactory  = MakeShared<FSkillTreePinFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(SkillNodeFactory);
	FEdGraphUtilities::RegisterVisualPinFactory(SkillPinFactory);

	IAssetTools& AssetTools =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	SkillAssetTypeActions = MakeShared<FAssetTypeActions_SkillTree>();
	AssetTools.RegisterAssetTypeActions(SkillAssetTypeActions.ToSharedRef());
}

void FGYEditorModule::ShutdownModule()
{
    if (DebugMenuManager.IsValid())
    {
        DebugMenuManager->Shutdown();
        DebugMenuManager.Reset();
    }

	FEdGraphUtilities::UnregisterVisualNodeFactory(SkillNodeFactory);
	FEdGraphUtilities::UnregisterVisualPinFactory(SkillPinFactory);

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools =
			FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(SkillAssetTypeActions.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGYEditorModule, GYEditor)
