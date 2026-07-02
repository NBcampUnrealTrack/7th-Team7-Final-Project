#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "GYPersistenceSubsystem.generated.h"

class FJsonObject;

enum class EGYPersistResult : uint8
{
	Success,
	NotFound, // Load: 캐릭터 행 없음
	Conflict, // Save: save_version 불일치 (RPC가 null 반환)
	Failure, // 연결/HTTP/파싱 실패 (타임아웃 포함)
};

struct FGYLoadResult
{
	EGYPersistResult Result = EGYPersistResult::Failure;
	int32 SaveVersion = 0;
	TSharedPtr<FJsonObject> Data;
};

struct FGYSaveResult
{
	EGYPersistResult Result = EGYPersistResult::Failure;
	int32 NewVersion = 0;
};

DECLARE_DELEGATE_OneParam(FGYOnLoadComplete, const FGYLoadResult&);
DECLARE_DELEGATE_OneParam(FGYOnSaveComplete, const FGYSaveResult&);

// 데디 서버에서 Supabase로 캐릭터 세이브를 load/save 하는 HTTP 전송기.
// 상태 없음 — 요청별 컨텍스트는 완료 델리게이트 캡처로 전달, 저장 상태(version/dirty)는 SaveManagerComponent가 소유.
// 요청에 타임아웃이 걸려 있어 완료 델리게이트는 반드시 한 번 호출됨 (hang 시 Failure).
// 설정은 UGYPersistenceSettings (Config/DefaultGYPersistence.ini). service_role 키는 RLS 우회 → 서버 전용, 클라 빌드에 절대 X.
UCLASS()
class GY_API UGYPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void LoadCharacter(int32 CharacterId, FGYOnLoadComplete OnComplete);
	void SaveCharacter(int32 CharacterId, int32 Level, int32 Xp, const FString& DataJson, int32 ExpectedVersion, FGYOnSaveComplete OnComplete);

	// 로그아웃 마무리 인계: EndPlay 시점에 in-flight 저장이 있으면 락에 막혀 최종 스냅샷을 못 보낸다.
	// 컴포넌트는 곧 죽으므로(재시도 불가) 수명이 긴 이 Subsystem 에 스냅샷을 맡긴다.
	// in-flight 완료 시 OnSaveCompletedInternal 이 이어받아 새 version 으로 전송.
	void HandoffLogoutSave(int32 CharacterId, int32 Level, int32 Xp, const FString& DataJson, int32 FallbackExpectedVersion);

	// Owner(PlayerState 등)의 IGYSaveable 컴포넌트들을 섹션 키로 묶어 data JSON 문자열로 직렬화
	FString CollectSaveData(AActor* Owner) const;

	// data JSON 의 각 섹션을 해당 IGYSaveable 컴포넌트로 복원 (서버 권위)
	void ApplySaveData(AActor* Owner, const TSharedPtr<FJsonObject>& DataObject);

private:
	// Initialize에서 ini를 한 번만 읽어 캐시 (config는 런타임에 안 변함 → 매 요청 디스크 I/O 회피)
	void LoadConfig();

	// 모든 SaveCharacter 완료가 거쳐가는 훅 — 인계분이 있으면 이어서 전송, 없으면 no-op
	void OnSaveCompletedInternal(int32 CharacterId, const FGYSaveResult& Result);

	struct FPendingLogoutSave
	{
		int32 Level = 1;
		int32 Xp = 0;
		FString DataJson;
		int32 FallbackExpectedVersion = 0;
	};

	// 로그아웃 인계 대기분 (캐릭터당 최신 하나). 전송기의 유일한 상태 —
	// 죽은 컴포넌트의 미완 저장을 이어받는 용도라 수명 긴 여기가 소유가 맞다
	TMap<int32, FPendingLogoutSave> PendingLogoutSaves;

	FString BaseUrl;
	FString SecretKey;
};
