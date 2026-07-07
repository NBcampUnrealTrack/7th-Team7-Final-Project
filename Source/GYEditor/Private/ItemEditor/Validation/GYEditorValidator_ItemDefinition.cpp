#include "ItemEditor/Validation/GYEditorValidator_ItemDefinition.h"

#include "ItemEditor/Validation/GYItemValidationContext.h"
#include "ItemEditor/Validation/GYItemValidationRules.h"

#include "Items/ItemDefinition.h"

#include "Misc/DataValidation.h"

bool UGYEditorValidator_ItemDefinition::CanValidateAsset_Implementation(
	const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject != nullptr && InObject->IsA<UItemDefinition>();
}

EDataValidationResult UGYEditorValidator_ItemDefinition::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UItemDefinition* Item = Cast<UItemDefinition>(InAsset);
	if (Item == nullptr) return EDataValidationResult::NotValidated;

	const FGYItemValidationContext ValidationContext = FGYItemValidationContext::BuildSingleAsset(Item);

	TArray<FGYItemValidationMessage> Messages;
	for (const TUniquePtr<IGYItemValidationRule>& Rule : MakeSaveTimeItemValidationRules())
	{
		Rule->Validate(ValidationContext, *Item, Messages);
	}

	for (const FGYItemValidationMessage& Message : Messages)
	{
		AssetWarning(InAsset, Message.Message);
	}

	AssetPasses(InAsset);
	return EDataValidationResult::Valid;
}
