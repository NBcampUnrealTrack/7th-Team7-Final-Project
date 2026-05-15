#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Quest/QuestTypes.h"
#include "QuestSubsystem.generated.h"

UCLASS()
class GY_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 퀘스트 정의 조회 (없으면 nullptr)
	const FQuestTableRow* FindQuestRow(FName QuestId) const;

	// 선행 퀘스트 조건 충족 여부 확인
	UFUNCTION(BlueprintCallable, Category="Quest")
	bool ArePrerequisitesMet(FName QuestId, const TArray<FQuestRuntimeData>& CompletedQuests) const;

	// 모든 목표 달성 여부 확인
	UFUNCTION(BlueprintCallable, Category="Quest")
	bool IsQuestComplete(const FQuestRuntimeData& RuntimeData) const;

private:
	// Row Name → Row 포인터 캐시 (DataTable 수명에 종속)
	TMap<FName, FQuestTableRow*> QuestCache;

	void BuildCache(UDataTable* DataTable);
};