#include "GYEditor.h"
#include "Debug/GYDebugMenuManager.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "Enemy/Abilities/GYEnemyComboAttack.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraphUtilities.h"
#include "Misc/PackageName.h"
#include "SkillTreeEditor/SkillTreeNodeFactory.h"
#include "SkillTreeEditor/AssetTypeActions_SkillTree.h"
#include "Enemy/AnimNotify/EnemyAttackState.h"
#include "Enemy/AnimNotify/LaunchProjectile.h"
#include "Misc/MessageDialog.h"

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

	// 변경이 실제로 필요할 때만 팝업
	if (!SyncAbilityWeightRow(BP, /*bDryRun=*/true))
		return;

	// 쿠킹/오토세이브 같은 절차적 저장에서는 모달을 띄울 수 없으므로 기존처럼 자동 적용
	if (Context.IsProceduralSave())
	{
		SyncAbilityWeightRow(BP);
		return;
	}

	const FText Message = FText::Format(
		LOCTEXT("SyncWeightRowPrompt",
			"'{0}' 어빌리티의 Trace Notify 개수가 변경되었습니다.\n"
			"EnemyAbilityWeights 테이블의 행을 수정하시겠습니까?"),
		FText::FromString(BP->GetName()));

	if (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes)
	{
		SyncAbilityWeightRow(BP);
	}
}

bool FGYEditorModule::SyncAbilityWeightRow(class UBlueprint* BP, bool bDryRun)
{
	if (!BP->GeneratedClass) return false;

	UGYEnemyAttackAbilityBase* CDO =
		Cast<UGYEnemyAttackAbilityBase>(BP->GeneratedClass->GetDefaultObject());
	if (!CDO) return false;

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
	if (!DataTable) return false;

	const FName RowName = *BP->GetName();

	FEnemyAbilityWeightRow NewRow;
	NewRow.AbilityClass = BP->GeneratedClass;

	if (const FEnemyAbilityWeightRow* Existing = DataTable->FindRow<FEnemyAbilityWeightRow>(RowName, TEXT("")))
	{
		if (Existing->HitDamageWeights.Num() == TraceCount &&
			Existing->AbilityClass.ToSoftObjectPath() == FSoftObjectPath(BP->GeneratedClass))
		{
			return false; // 변경 필요 없음
		}
		NewRow.HitDamageWeights = Existing->HitDamageWeights;
		NewRow.ActivateCost = Existing->ActivateCost;
	}

	if (bDryRun) return true; // 수정이 필요하지만 적용은 안 함

	NewRow.HitDamageWeights.SetNum(TraceCount);

	DataTable->AddRow(RowName, NewRow);
	DataTable->MarkPackageDirty();
	return true;
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

	// 개별 팝업이 연달아 뜨지 않도록 수정이 필요한 BP를 먼저 모은 뒤 한 번만 묻는다
	TArray<UBlueprint*> PendingBPs;

	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassPath;
		if (!Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassPath)) continue;

		const FTopLevelAssetPath ClassPath(
			FPackageName::ExportTextPathToObjectPath(GeneratedClassPath));
		if (!DerivedClasses.Contains(ClassPath)) continue;

		if (UBlueprint* BP = Cast<UBlueprint>(Asset.GetAsset()))
		{
			if (SyncAbilityWeightRow(BP, /*bDryRun=*/true))
			{
				PendingBPs.Add(BP);
			}
		}
	}

	if (PendingBPs.IsEmpty()) return;

	FString NameList;
	for (const UBlueprint* BP : PendingBPs)
	{
		NameList += FString::Printf(TEXT("\n - %s"), *BP->GetName());
	}

	const FText Message = FText::Format(
		LOCTEXT("SyncAllWeightRowsPrompt",
			"EnemyAbilityWeights 테이블과 어긋난 어빌리티가 {0}개 있습니다.{1}\n\n테이블을 수정하시겠습니까?"),
		PendingBPs.Num(), FText::FromString(NameList));

	if (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes)
	{
		for (UBlueprint* BP : PendingBPs)
		{
			SyncAbilityWeightRow(BP);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGYEditorModule, GYEditor)
