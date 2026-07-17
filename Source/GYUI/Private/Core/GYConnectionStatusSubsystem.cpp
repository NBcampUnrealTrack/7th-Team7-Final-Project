#include "Core/GYConnectionStatusSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "Widget/Common/GYMessagePopupWidget.h"

#define LOCTEXT_NAMESPACE "GYUI"

bool UGYConnectionStatusSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 데디 서버 인스턴스에서는 불필요
	if (const UGameInstance* GI = Cast<UGameInstance>(Outer))
	{
		return !GI->IsDedicatedServerInstance();
	}
	return true;
}

void UGYConnectionStatusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
			this, &UGYConnectionStatusSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
			this, &UGYConnectionStatusSubsystem::HandleTravelFailure);
	}
}

void UGYConnectionStatusSubsystem::Deinitialize()
{
	if (GEngine)
	{
		if (NetworkFailureHandle.IsValid()) GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		if (TravelFailureHandle.IsValid())  GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	Super::Deinitialize();
}

void UGYConnectionStatusSubsystem::HandleNetworkFailure(UWorld*, UNetDriver*,
	ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 인게임 연결 끊김 -> 엔진이 기본 맵으로 트래블, 트래블 완료 후 팝업
	if (FailureType == ENetworkFailure::PendingConnectionFailure && ErrorString == TEXT("world_full"))
	{
		// 서버 PreLogin 의 인원 초과 거부 (GYGameMode) — 일반 끊김과 구분해 안내
		PendingTitle   = LOCTEXT("WorldFull_Title", "월드 만석");
		PendingMessage = LOCTEXT("WorldFull_Message", "월드 인원이 가득 찼습니다. (최대 4명)\n자리가 나면 다시 시도해주세요.");
	}
	else
	{
		PendingTitle   = LOCTEXT("Disconnect_Title", "연결 끊김");
		PendingMessage = LOCTEXT("Disconnect_Message", "서버와의 연결이 끊어졌습니다.\n메인 메뉴로 돌아갑니다.");
	}
	bHasPending = true;

	if (!PostLoadMapHandle.IsValid())
	{
		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
			this, &UGYConnectionStatusSubsystem::HandlePostLoadMap);
	}
}

void UGYConnectionStatusSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (bHasPending && LoadedWorld)
	{
		bHasPending = false;
		UGYMessagePopupWidget::ShowNotice(LoadedWorld, PendingTitle, PendingMessage);
	}
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
}

void UGYConnectionStatusSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type, const FString&)
{
	// 접속 실패 - 보통 메뉴에 그대로 있으므로 즉시 표시
	const UObject* Ctx = World ? static_cast<const UObject*>(World) : static_cast<const UObject*>(GetGameInstance());
	UGYMessagePopupWidget::ShowNotice(Ctx,
		LOCTEXT("Travel_Failed_Title", "접속 실패"),
		LOCTEXT("Travel_Failed_Message", "서버에 접속하지 못했습니다."));
}

#undef LOCTEXT_NAMESPACE
