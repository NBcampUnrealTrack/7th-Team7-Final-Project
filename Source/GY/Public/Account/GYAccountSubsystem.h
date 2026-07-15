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

struct FGYWorldSummary
{
	int64 Id = 0;
	FString Name;
	int32 WorldLevel = 1;
	FString Status; // offline | starting | online
	FString HostAddr;
	int32 PlayerCount = 0;
	int32 MaxPlayers = 4;
	bool IsJoinable() const { return Status == TEXT("online") && PlayerCount < MaxPlayers && !HostAddr.IsEmpty(); }
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGYOnAccountReady, bool /*bSuccess*/);
DECLARE_DELEGATE_TwoParams(FGYOnCharacterList, bool /*bSuccess*/, const TArray<FGYCharacterSummary>&);
DECLARE_DELEGATE_TwoParams(FGYOnCharacterOp, bool /*bSuccess*/, int64 /*CharacterId*/);
DECLARE_DELEGATE_TwoParams(FGYOnWorldList, bool /*bSuccess*/, const TArray<FGYWorldSummary>&);
DECLARE_DELEGATE_TwoParams(FGYOnWorldOp, bool /*bSuccess*/, int64 /*WorldId*/);
// Phase: 진행 단계 통지 (UI 표시용). Requested → Starting → Online(접속 개시) / Failed
enum class EGYJoinWorldPhase : uint8 { Requested, Starting, Online, Failed };
DECLARE_MULTICAST_DELEGATE_TwoParams(FGYOnJoinWorldPhase, int64 /*WorldId*/, EGYJoinWorldPhase);

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

	// 계정-캐릭터 1:1 운영(캐릭터 선택 UI 전): 로그인 시 자동 확보되는 내 캐릭터. 0 = 미확보
	int64 GetPrimaryCharacterId() const { return PrimaryCharacterId; }

	// 로그인 완료(성공/실패) 통지. 재로그인 시에도 매번 브로드캐스트
	FGYOnAccountReady OnAccountReady;

	// 캐릭터 메타 CRUD (PostgREST + Bearer). 세이브 내용(level/xp/data)은 데디 전용 — 여긴 목록/생성/삭제만
	void ListCharacters(FGYOnCharacterList OnComplete);
	void CreateCharacter(const FString& CharacterName, FGYOnCharacterOp OnComplete);
	void DeleteCharacter(int64 CharacterId, FGYOnCharacterOp OnComplete);

	// ── 월드 목록 조회/생성/입장 (같은 Bearer 경로) ──
	void ListWorlds(FGYOnWorldList OnComplete);

	// 내 소유 월드 생성 (이름만 — 레벨/상태는 기본값, 계정당 상한은 DB 백스톱)
	void CreateWorld(const FString& WorldName, FGYOnWorldOp OnComplete);

	// 목록의 월드에 입장하는 단일 진입점: online 이면 즉시 접속, offline 이면
	// 시작 요청(request_world_start) → online 폴링 → 접속. 진행 단계는 OnJoinWorldPhase 로 통지
	void JoinWorld(int64 WorldId);

	FGYOnJoinWorldPhase OnJoinWorldPhase;

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

	// 1:1 운영: 첫 캐릭터 채택, 없으면 생성. 완료 후에야 OnAccountReady 브로드캐스트 —
	// 캐릭터 선택 UI 도입 시 이 자동 채택만 UI 선택으로 교체하면 됨
	void EnsurePrimaryCharacter();

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

	// JoinWorld 폴링 루프 — 시작 요청 후 online 전환 감시 (스폰 실패 대비 타임아웃)
	void PollJoinTarget();
	void FinishJoin(const FGYWorldSummary& World);
	void FailJoin(const TCHAR* Reason);

	FString AccountId;
	FString AccessToken;
	FString RefreshToken;
	FString PersonaName;
	int64 PrimaryCharacterId = 0;
	// FPlatformTime::Seconds() 기준 만료 시각 — 갱신 판단용
	double TokenExpiresAtSeconds = 0.0;
	bool bLoginInFlight = false;
	bool bRefreshInFlight = false;

	int64 JoinTargetWorldId = 0;
	double JoinDeadlineSeconds = 0.0;
	FTimerHandle JoinPollTimerHandle;
};
