#pragma once

#include "Engine/DeveloperSettings.h"
#include "GYPersistenceSettings.generated.h"

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

	// service_role 키 — RLS 우회, 서버 전용. 클라 빌드에 절대 X.
	UPROPERTY(EditDefaultsOnly, Config, Category="Persistence")
	FString ServiceRoleKey;
};
