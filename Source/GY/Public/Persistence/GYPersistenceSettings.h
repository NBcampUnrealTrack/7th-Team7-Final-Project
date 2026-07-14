#pragma once

#include "Engine/DeveloperSettings.h"
#include "GYPersistenceSettings.generated.h"

// 클라 신원 모드. 에디터 기본 Mock — OSS Steam 신원은 PIE 에서도 동작하지만
// 에디터발 인스턴스는 전부 같은 SteamID 라서 다계정 테스트는 Mock 으로.
UENUM()
enum class EGYAuthMode : uint8
{
	Mock,
	Steam,
};

// Config=GYPersistence → Config/DefaultGYPersistence.ini 에 자동 바인딩.
// 로컬 demo 값은 커밋됨. 패키징/배포는 상위 ini/CI/env 로 override (호스티드 키는 커밋 금지).
UCLASS(Config=GYPersistence, DefaultConfig, meta=(DisplayName="GY Persistence"))
class GY_API UGYPersistenceSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Project Settings 의 "Project" 섹션에 표시 (기본은 Engine)
	virtual FName GetCategoryName() const override { return FName(TEXT("Project")); }

	UPROPERTY(EditDefaultsOnly, Config, Category="Persistence")
	FString ServerBaseUrl;

	// Supabase Secret key (service_role 등가) — RLS 우회, 서버 전용. 클라 빌드에 절대 X.
	UPROPERTY(EditDefaultsOnly, Config, Category="Persistence")
	FString SecretKey;

	// Supabase Publishable key (anon 등가) — 클라 로그인/캐릭터 CRUD 용. RLS 적용이라 노출 가능
	UPROPERTY(EditDefaultsOnly, Config, Category="Account")
	FString PublishableKey;

	// -AuthMode= 커맨드라인 및 gy.Account.Login [Mock|Steam] 으로 override 가능
	UPROPERTY(EditDefaultsOnly, Config, Category="Account")
	EGYAuthMode AuthMode = EGYAuthMode::Mock;

	// Mock 신원 접두 — PIE 인스턴스 번호가 접미로 붙어 dev_test_0/1/... 로 계정 분리. -MockSteamId= override 가능
	UPROPERTY(EditDefaultsOnly, Config, Category="Account")
	FString MockSteamId = TEXT("dev_test");
};
