#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "ItemEditor/Validation/GYItemValidationTypes.h"
#include "UObject/GCObject.h"
#include "UObject/ObjectKey.h"

class IGYItemValidationRule;
class UDataTable;
class UItemDefinition;
class URegionLootData;
struct FAssetData;
struct FPropertyChangedEvent;

struct FGYPoolRowValues
{
	int32 Weight = 1;
	int32 MinCount = 1;
	int32 MaxCount = 1;
};

struct FGYPoolMembership
{
	TWeakObjectPtr<UDataTable> Pool;
	bool bMember = false;
	FGYPoolRowValues Values;
};

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

	// 베이스 스탯 행 (행 키 = ItemId)
	UDataTable* ResolveBaseStatsTable(const UItemDefinition* Item) const;
	bool CreateBaseStatsRow(UItemDefinition* Item);
	bool DeleteBaseStatsRow(UItemDefinition* Item);

	// 아이템 풀 소속
	TArray<FGYPoolMembership> QueryPoolMembership(const UItemDefinition* Item) const;
	TOptional<FGYPoolRowValues> GetPoolRowValues(const UItemDefinition* Item, UDataTable* Pool) const;
	void SetPoolMembership(UItemDefinition* Item, UDataTable* Pool, bool bMember);
	void SetPoolRowValues(UItemDefinition* Item, UDataTable* Pool, const FGYPoolRowValues& Values);
	TArray<FText> QueryRegionExposure(const UItemDefinition* Item) const;

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

	// 마지막 검증에서 참조한 DT/Region. GC 방지 + 저장/편집 대상 조회용
	TObjectPtr<UDataTable> WeaponStatsTable;
	TObjectPtr<UDataTable> ArmorStatsTable;
	TArray<TObjectPtr<UDataTable>> ItemPools;
	TArray<TObjectPtr<URegionLootData>> Regions;

	bool bRefreshQueued = false;
	bool bRescanQueued = false;

	FDelegateHandle AssetAddedHandle;
	FDelegateHandle AssetRemovedHandle;
	FDelegateHandle AssetRenamedHandle;
	FDelegateHandle PropertyChangedHandle;
};
