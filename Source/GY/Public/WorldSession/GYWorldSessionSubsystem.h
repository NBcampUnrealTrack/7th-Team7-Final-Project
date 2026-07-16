#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/Function.h"
#include "GYWorldSessionSubsystem.generated.h"

struct FGYWorldSummary
{
	int64 Id = 0;
	FString Name;
	int32 WorldLevel = 1;
	FString Status; // offline | starting | online
	FString HostAddr;
	int32 PlayerCount = 0;
	int32 MaxPlayers = 4;
	FString OwnerAccountId; // 소유 계정 (시스템 월드는 빈 값) — 캐릭터는 계정 안에서 교체 가능하므로 소유/참여 주체는 계정
	FString OwnerName; // 생성 시점 persona (denorm)
	bool bParticipant = false; // 내 계정의 참여 기록 존재 (world_participants)
	bool IsJoinable() const { return Status == TEXT("online") && PlayerCount < MaxPlayers && !HostAddr.IsEmpty(); }
};

DECLARE_DELEGATE_TwoParams(FGYOnWorldList, bool /*bSuccess*/, const TArray<FGYWorldSummary>&);
DECLARE_DELEGATE_TwoParams(FGYOnWorldOp, bool /*bSuccess*/, int64 /*WorldId*/);
// 스탠바이 등록(id)/배정 폴링(world id, 0 = 대기) 응답
DECLARE_DELEGATE_TwoParams(FGYOnSessionId, bool /*bSuccess*/, int64 /*Id*/);

// Phase: 진행 단계 통지 (UI 표시용). Requested → (Queued) → Starting → Online(접속 개시) / Failed
// Queued = 다른 월드가 활성이라 서버 슬롯 대기 중 (오케스트레이터 MaxWorlds) — 슬롯이 비면 자동 진행
enum class EGYJoinWorldPhase : uint8 { Requested, Queued, Starting, Online, Failed };
DECLARE_MULTICAST_DELEGATE_TwoParams(FGYOnJoinWorldPhase, int64 /*WorldId*/, EGYJoinWorldPhase);

// 월드 세션(worlds/standby_servers 테이블) 도메인 — 목록/생성/입장(클라)과 하트비트/스탠바이(서버)를 한곳에.
// 클라 경로는 AccountSubsystem 의 Bearer 토큰(RLS 적용)을 빌려 쓰고,
// 서버 경로는 GYPersistence.ini 의 service_role 키(RLS 우회, 서버 전용)를 직접 쓴다.
// 신원은 Account, 세이브 데이터는 Persistence — 이 서브시스템은 세션 수명주기만 담당한다.
UCLASS()
class GY_API UGYWorldSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ── 클라 경로 (Bearer — 로그인 필요) ──
	void ListWorlds(FGYOnWorldList OnComplete);

	// 내 소유 월드 생성 (이름만 — 레벨/상태는 기본값, 계정당 상한은 DB 백스톱)
	void CreateWorld(const FString& WorldName, FGYOnWorldOp OnComplete);

	// 목록의 월드에 입장하는 단일 진입점: online 이면 즉시 접속, offline 이면
	// 시작 요청(request_world_start) → online 폴링 → 접속. 진행 단계는 OnJoinWorldPhase 로 통지
	void JoinWorld(int64 WorldId);

	// 진행 중인 입장 취소 (대기열/준비 중 UI 의 취소 버튼). 서버 쪽 철회는 불필요 —
	// 스폰돼 버린 서버는 유휴 회수가 정리한다. Failed 페이즈로 통지되어 UI 가 원상복구된다
	void CancelJoin();

	bool IsJoinInProgress() const { return JoinTargetWorldId != 0; }

	FGYOnJoinWorldPhase OnJoinWorldPhase;

	// ── 서버 경로 (service_role — fire-and-forget, 실패는 하트비트/오케스트레이터 리핑이 수습) ──
	void HeartbeatWorld(int64 WorldId, const FString& PublicAddr, int32 PlayerCount);
	void SetWorldOffline(int64 WorldId);

	// 월드 참여자 기록 — 클라 "참가 중인 월드" 분류의 근거 (권한 아님, 노출용)
	void RecordWorldParticipant(int64 WorldId, int64 CharacterId);

	// ── 웜 스탠바이 (WorldSessionComponent 전용) ──
	void RegisterStandby(const FString& PublicAddr, FGYOnSessionId OnComplete);
	void PollStandbyAssignment(int64 StandbyId, FGYOnSessionId OnComplete);
	void StandbyHeartbeat(int64 StandbyId);
	void ConsumeStandby(int64 StandbyId);

private:
	// Initialize에서 ini를 한 번만 읽어 캐시 — 서버 경로(service_role)용. 클라 빌드에선 키가 비어 no-op
	void LoadConfig();

	// service_role 요청 공통 경로 (POST RPC). OnDone 미지정이면 fire-and-forget
	void SendServiceRequest(const FString& Path, const FString& BodyJson,
		TFunction<void(int32 Code, const FString& Content)> OnDone = nullptr);

	// JoinWorld 폴링 루프 — 시작 요청 후 online 전환 감시 (스폰 실패 대비 타임아웃)
	void PollJoinTarget();
	void FinishJoin(const FGYWorldSummary& World);
	void FailJoin(const TCHAR* Reason);

	class UGYAccountSubsystem* ResolveAccount() const;

	FString BaseUrl;
	FString SecretKey;

	int64 JoinTargetWorldId = 0;
	double JoinDeadlineSeconds = 0.0;
	FTimerHandle JoinPollTimerHandle;
};
