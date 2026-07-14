#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/Function.h"
#include "Persistence/GYPersistenceSettings.h"
#include "GYAccountSubsystem.generated.h"

struct FGYCharacterSummary
{
	int64 Id = 0;
	FString Name;
	int32 Level = 1;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGYOnAccountReady, bool /*bSuccess*/);
DECLARE_DELEGATE_TwoParams(FGYOnCharacterList, bool /*bSuccess*/, const TArray<FGYCharacterSummary>&);
DECLARE_DELEGATE_TwoParams(FGYOnCharacterOp, bool /*bSuccess*/, int64 /*CharacterId*/);

// 클라이언트 신원 주체 — Mock/Steam 신원 해석 → steam-auth Edge Function → GoTrue 토큰 보유.
// 캐릭터 CRUD 는 이 토큰(Bearer) + PublishableKey 로 PostgREST 직접 호출.
// 데디 저장 경로(GYPersistenceSubsystem, SecretKey)와 완전 별개 — 여긴 RLS 가 적용되는 클라 경로.
UCLASS()
class GY_API UGYAccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 설정(ini)/커맨드라인(-AuthMode=)의 모드로 로그인
	void Login();
	void LoginWithMode(EGYAuthMode Mode);

	bool IsLoggedIn() const { return !AccessToken.IsEmpty(); }
	const FString& GetAccountId() const { return AccountId; }
	const FString& GetAccessToken() const { return AccessToken; }
	const FString& GetPersonaName() const { return PersonaName; }

	// 로그인 완료(성공/실패) 통지. 재로그인 시에도 매번 브로드캐스트
	FGYOnAccountReady OnAccountReady;

	// 캐릭터 메타 CRUD (PostgREST + Bearer). 세이브 내용(level/xp/data)은 데디 전용 — 여긴 목록/생성/삭제만
	void ListCharacters(FGYOnCharacterList OnComplete);
	void CreateCharacter(const FString& CharacterName, FGYOnCharacterOp OnComplete);
	void DeleteCharacter(int64 CharacterId, FGYOnCharacterOp OnComplete);

private:
	struct FGYResolvedIdentity
	{
		FString SteamId;
		FString Ticket;
		FString PersonaName;
	};

	void LoadSettings();
	bool ResolveIdentity(EGYAuthMode Mode, FGYResolvedIdentity& OutIdentity) const;
	void RequestAuth(const FGYResolvedIdentity& Identity);

	// Bearer 토큰 요청 공통 경로 — 401 이면 refresh 후 1회 재시도, refresh 실패 시 세션 클리어.
	// 재시도를 위해 요청 내용을 람다에 보관하므로 값 전달(sink)
	void SendAuthedRequest(FString Verb, FString Path, FString ContentJson,
		FString PreferHeader, TFunction<void(int32 Code, const FString& Content)> OnDone, bool bIsRetry = false);
	void RefreshSession(TFunction<void(bool bSuccess)> OnDone);
	void ClearSession();

	FString BaseUrl;
	FString PublishableKey;
	EGYAuthMode DefaultAuthMode = EGYAuthMode::Mock;
	FString MockSteamIdPrefix;

	FString AccountId;
	FString AccessToken;
	FString RefreshToken;
	FString PersonaName;
	// FPlatformTime::Seconds() 기준 만료 시각 — 갱신 판단용
	double TokenExpiresAtSeconds = 0.0;
	bool bLoginInFlight = false;
	bool bRefreshInFlight = false;
};
