#pragma once

#include "UObject/Interface.h"
#include "GYSaveable.generated.h"

class FJsonValue;

UINTERFACE(MinimalAPI)
class UGYSaveable : public UInterface
{
	GENERATED_BODY()
};

// 세이브 직렬화를 컴포넌트별로 담당하는 인터페이스 (컴포넌트별 직렬화 구조).
// PersistenceSubsystem 이 PlayerState 의 IGYSaveable 컴포넌트들을 모아 섹션 키로 묶는다.
class GY_API IGYSaveable
{
	GENERATED_BODY()

public:
	// 세이브 JSON 의 섹션 키 (예: "currency"). 섹션끼리 고유해야 한다.
	virtual FString GetSaveSectionKey() const = 0;

	// 이 컴포넌트 상태를 JSON 값으로 내보냄 (object/array).
	virtual TSharedPtr<FJsonValue> ExportSaveData() const = 0;

	// JSON 값에서 상태 복원 (서버 권위에서만 호출).
	virtual void ImportSaveData(const TSharedPtr<FJsonValue>& Data) = 0;
};
