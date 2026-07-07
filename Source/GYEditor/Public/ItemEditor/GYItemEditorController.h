#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "ItemEditor/Validation/GYItemValidationTypes.h"
#include "UObject/GCObject.h"
#include "UObject/ObjectKey.h"

class IGYItemValidationRule;
class UItemDefinition;
struct FAssetData;
struct FPropertyChangedEvent;

// 아이템 에디터의 데이터 허브. 에셋 스캔·밸리데이션·저장을 담당하고 UI는 이 결과만 그린다.
class FGYItemEditorController
	: public TSharedFromThis<FGYItemEditorController>
	, public FGCObject
	, public FEditorUndoClient
{
public:
	// TUniquePtr<IGYItemValidationRule> 생성/소멸에 완전한 타입이 필요해 cpp에서 정의
	FGYItemEditorController();
	~FGYItemEditorController();

	void Initialize();
	void Shutdown();

	const TArray<TObjectPtr<UItemDefinition>>& GetItems() const { return Items; }
	const FGYItemValidationReport* FindReport(const UItemDefinition* Item) const;
	const TArray<FGYItemValidationMessage>& GetGlobalIssues() const { return GlobalIssues; }

	void RefreshAll();
	void ValidateAll();
	void SaveAllDirty();

	FSimpleMulticastDelegate OnDataChanged;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:
	void ScanItems();
	void RequestRefresh(bool bRescanAssets);
	void FlushRefresh();
	void HandleAssetAddedOrRemoved(const FAssetData& AssetData);
	void HandleAssetRenamed(const FAssetData& AssetData, const FString& OldPath);
	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);

	TArray<TObjectPtr<UItemDefinition>> Items;
	TMap<FObjectKey, FGYItemValidationReport> Reports;
	TArray<FGYItemValidationMessage> GlobalIssues;
	TArray<TUniquePtr<IGYItemValidationRule>> Rules;

	// 마지막 검증에서 참조한 DT/Region. GC 방지 + 일괄 저장 대상 수집용
	TArray<TObjectPtr<UObject>> RelatedAssets;

	bool bRefreshQueued = false;
	bool bRescanQueued = false;

	FDelegateHandle AssetAddedHandle;
	FDelegateHandle AssetRemovedHandle;
	FDelegateHandle AssetRenamedHandle;
	FDelegateHandle PropertyChangedHandle;
};
