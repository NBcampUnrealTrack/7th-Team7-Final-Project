#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
class FSkillTreeNodeFactory;
class FSkillTreePinFactory;
class FAssetTypeActions_SkillTree;

class FGYDebugMenuManager;

class FGYEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
	TSharedPtr<FSkillTreeNodeFactory>       SkillNodeFactory;
	TSharedPtr<FSkillTreePinFactory>        SkillPinFactory;
	TSharedPtr<FAssetTypeActions_SkillTree> SkillAssetTypeActions;

    TSharedPtr<FGYDebugMenuManager> DebugMenuManager;
};
