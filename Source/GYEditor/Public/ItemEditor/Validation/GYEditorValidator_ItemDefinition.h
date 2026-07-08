#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "GYEditorValidator_ItemDefinition.generated.h"

// UItemDefinition 저장/검증 시 자동 실행되는 validator. 아이템 에디터의 규칙 엔진을 재사용한다.
// 도입 초기라 결과는 전부 경고로만 표시하고 저장을 막지 않는다.
UCLASS()
class UGYEditorValidator_ItemDefinition : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
};
