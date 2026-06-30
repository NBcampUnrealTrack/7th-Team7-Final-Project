#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "GYPersistenceSubsystem.generated.h"

// 데디 서버에서 Supabase로 캐릭터 세이브를 load/save 하는 HTTP 클라이언트.
// 설정은 UGYPersistenceSettings (Config/DefaultGYPersistence.ini). 로컬 demo 값은 커밋됨.
// 호스티드 실서버 키는 커밋 금지(CI/env/상위 ini로 override). service_role 은 RLS 우회 → 서버 전용, 클라 빌드에 절대 X.
// 콘솔(gy.Persist.Load / gy.Persist.Save)로 수동 테스트. 실제 입장/저장 연동은 후속.
UCLASS()
class GY_API UGYPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void LoadCharacter(int32 CharacterId, AActor* ApplyTarget = nullptr);
	void SaveCharacter(int32 CharacterId, int32 Level, int32 Xp, const FString& DataJson, int32 ExpectedVersion);

	// Owner(PlayerState 등)의 IGYSaveable 컴포넌트들을 섹션 키로 묶어 data JSON 문자열로 직렬화
	FString CollectSaveData(AActor* Owner) const;

	// data JSON 의 각 섹션을 해당 IGYSaveable 컴포넌트로 복원 (서버 권위)
	void ApplySaveData(AActor* Owner, const TSharedPtr<class FJsonObject>& DataObject);

	int32 GetCachedSaveVersion() const { return CachedSaveVersion; }

private:
	// Initialize에서 ini를 한 번만 읽어 캐시 (config는 런타임에 안 변함 → 매 요청 디스크 I/O 회피)
	void LoadConfig();

	void OnLoadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnSaveComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	FString BaseUrl;
	FString SecretKey;

	// Load 응답 도착 시 ApplySaveData 대상 (LoadCharacter 호출 시점에 지정)
	TWeakObjectPtr<AActor> PendingApplyTarget;

	// 마지막으로 load/save 한 save_version. 콘솔 Save가 expectedVersion으로 사용
	int32 CachedSaveVersion = 0;
};
