#include "GYEditor.h"
#include "Debug/GYDebugMenuManager.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "SkillTreeEditor/SkillTreeNodeFactory.h"
#include "SkillTreeEditor/AssetTypeActions_SkillTree.h"
#include "Enemy/AnimNotify/EnemyWeaponTrace.h"
#include "Enemy/AnimNotify/LaunchProjectile.h"

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

	OnObjectPreSaveHandle = FCoreUObjectDelegates::OnObjectPreSave.AddRaw(
		this, &FGYEditorModule::OnObjectPreSave
	);
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

	FCoreUObjectDelegates::OnObjectPreSave.Remove(OnObjectPreSaveHandle);
}

void FGYEditorModule::OnObjectPreSave(UObject* Object, FObjectPreSaveContext Context)
{
	UBlueprint* BP = Cast<UBlueprint>(Object);
	if (!BP) return;

	if (!BP->ParentClass || !BP->ParentClass->IsChildOf(UGYEnemyAttackAbilityBase::StaticClass()))
		return;

	SyncAbilityWeightRow(BP);
}

void FGYEditorModule::SyncAbilityWeightRow(class UBlueprint* BP)
{
	if (!BP->GeneratedClass) return;

	UGYEnemyAttackAbilityBase* CDO =
		Cast<UGYEnemyAttackAbilityBase>(BP->GeneratedClass->GetDefaultObject());
	if (!CDO) return;

	if (CDO->AttackType != EGYEnemyAttackType::Melee &&
		CDO->AttackType != EGYEnemyAttackType::Ranged)
	{
		return;
	}

	int32 TraceCount = 0;
	if (CDO->AttackMontage)
	{
		for (const FAnimNotifyEvent& NotifyEvent : CDO->AttackMontage->Notifies)
		{
			if (NotifyEvent.NotifyStateClass &&
				NotifyEvent.NotifyStateClass->IsA<UEnemyWeaponTrace>())
			{
				TraceCount++;
				continue;
			}

			if (NotifyEvent.Notify && NotifyEvent.Notify->IsA<ULaunchProjectile>())
			{
				TraceCount++;
				continue;
			}
		}
	}

	UDataTable* DataTable = LoadObject<UDataTable>(nullptr,
		TEXT("/Game/GY/Data/Tables/EnemyAbilityWeights"));
	if (!DataTable) return;

	FName RowName = *BP->GetName();

	FEnemyAbilityWeightRow NewRow;
	NewRow.AbilityClass = BP->GeneratedClass;

	if (FEnemyAbilityWeightRow* Existing = DataTable->FindRow<FEnemyAbilityWeightRow>(RowName, TEXT("")))
	{
		NewRow.HitDamageWeights = Existing->HitDamageWeights;
	}

	const int32 OldCount = NewRow.HitDamageWeights.Num();
	NewRow.HitDamageWeights.SetNum(TraceCount);

	DataTable->AddRow(RowName, NewRow);
	DataTable->MarkPackageDirty();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGYEditorModule, GYEditor)
