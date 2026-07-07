#include "GYEditor.h"
#include "Debug/GYDebugMenuManager.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "Enemy/Abilities/GYEnemyComboAttack.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraphUtilities.h"
#include "Misc/PackageName.h"
#include "ItemEditor/GYItemEditorRegistration.h"
#include "SkillTreeEditor/SkillTreeNodeFactory.h"
#include "SkillTreeEditor/AssetTypeActions_SkillTree.h"
#include "Enemy/AnimNotify/EnemyAttackState.h"
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

	FGYItemEditorRegistration::Register();

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	if (AssetRegistry.IsLoadingAssets())
	{
		OnFilesLoadedHandle = AssetRegistry.OnFilesLoaded().AddRaw(
			this, &FGYEditorModule::SyncAllAbilityWeightRows);
	}
	else
	{
		SyncAllAbilityWeightRows();
	}
}

void FGYEditorModule::ShutdownModule()
{
	FGYItemEditorRegistration::Unregister();

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

	if (OnFilesLoadedHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry")
			.Get().OnFilesLoaded().Remove(OnFilesLoadedHandle);
	}
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

	int32 TraceCount = UGYEnemyAttackAbilityBase::CountTraceNotifies(CDO->AttackMontage);

	if (const UGYEnemyComboAttack* Combo = Cast<UGYEnemyComboAttack>(CDO))
	{
		for (const FComboStep& Step : Combo->ComboSteps)
		{
			TraceCount += UGYEnemyAttackAbilityBase::CountTraceNotifies(Step.Montage);
		}
	}

	TraceCount = FMath::Max(TraceCount, 1);

	UDataTable* DataTable = LoadObject<UDataTable>(nullptr,
		TEXT("/Game/GY/Data/Tables/EnemyAbilityWeights"));
	if (!DataTable) return;

	const FName RowName = *BP->GetName();

	FEnemyAbilityWeightRow NewRow;
	NewRow.AbilityClass = BP->GeneratedClass;

	if (const FEnemyAbilityWeightRow* Existing = DataTable->FindRow<FEnemyAbilityWeightRow>(RowName, TEXT("")))
	{
		if (Existing->HitDamageWeights.Num() == TraceCount &&
			Existing->AbilityClass.ToSoftObjectPath() == FSoftObjectPath(BP->GeneratedClass))
		{
			return;
		}
		NewRow.HitDamageWeights = Existing->HitDamageWeights;
		NewRow.ActivateCost = Existing->ActivateCost;
	}

	NewRow.HitDamageWeights.SetNum(TraceCount);

	DataTable->AddRow(RowName, NewRow);
	DataTable->MarkPackageDirty();
}

void FGYEditorModule::SyncAllAbilityWeightRows()
{
	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TSet<FTopLevelAssetPath> DerivedClasses;
	AssetRegistry.GetDerivedClassNames(
		{ UGYEnemyAttackAbilityBase::StaticClass()->GetClassPathName() },
		TSet<FTopLevelAssetPath>(), DerivedClasses);

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassPath;
		if (!Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassPath)) continue;

		const FTopLevelAssetPath ClassPath(
			FPackageName::ExportTextPathToObjectPath(GeneratedClassPath));
		if (!DerivedClasses.Contains(ClassPath)) continue;

		if (UBlueprint* BP = Cast<UBlueprint>(Asset.GetAsset()))
		{
			SyncAbilityWeightRow(BP);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGYEditorModule, GYEditor)
