#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectSaveContext.h"
#include "Enemy/DataTables/EnemyAbilityWeightRow.h"
#include "Enemy/AnimNotify/EnemyWeaponTrace.h"
#include "Engine/DataTable.h"
#include "Engine/Blueprint.h"
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
	void OnObjectPreSave(UObject* Object, FObjectPreSaveContext Context);
	void SyncAbilityWeightRow(class UBlueprint* BP);

	FDelegateHandle OnObjectPreSaveHandle;
private:
	TSharedPtr<FSkillTreeNodeFactory>       SkillNodeFactory;
	TSharedPtr<FSkillTreePinFactory>        SkillPinFactory;
	TSharedPtr<FAssetTypeActions_SkillTree> SkillAssetTypeActions;

    TSharedPtr<FGYDebugMenuManager> DebugMenuManager;
};
