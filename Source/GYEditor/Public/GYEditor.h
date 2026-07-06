#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectSaveContext.h"
#include "Enemy/DataTables/EnemyAbilityWeightRow.h"
#include "Enemy/AnimNotify/EnemyAttackState.h"
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

	// 에디터 시작 시 기존 어빌리티 BP 전체를 테이블에 백필 (저장된 적 없는 BP도 행이 생기도록)
	void SyncAllAbilityWeightRows();

	FDelegateHandle OnObjectPreSaveHandle;
	FDelegateHandle OnFilesLoadedHandle;
private:
	TSharedPtr<FSkillTreeNodeFactory>       SkillNodeFactory;
	TSharedPtr<FSkillTreePinFactory>        SkillPinFactory;
	TSharedPtr<FAssetTypeActions_SkillTree> SkillAssetTypeActions;

    TSharedPtr<FGYDebugMenuManager> DebugMenuManager;
};
