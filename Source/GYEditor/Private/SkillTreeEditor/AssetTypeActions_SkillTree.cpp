#include "SkillTreeEditor/AssetTypeActions_SkillTree.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "SkillTreeEditor/SkillTreeAssetEditor.h"

FText FAssetTypeActions_SkillTree::GetName() const
{
	return NSLOCTEXT("SkillTree", "Name", "Skill Tree");
}

FColor FAssetTypeActions_SkillTree::GetTypeColor() const
{
	return FColor(0, 200, 100);
}

UClass* FAssetTypeActions_SkillTree::GetSupportedClass() const
{
	return USkillTreeDataAsset::StaticClass();
}

uint32 FAssetTypeActions_SkillTree::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

void FAssetTypeActions_SkillTree::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		if (USkillTreeDataAsset* Asset = Cast<USkillTreeDataAsset>(Obj))
		{
			TSharedRef<FSkillTreeAssetEditor> Editor = MakeShared<FSkillTreeAssetEditor>();
			Editor->InitEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Asset);
		}
	}
}
