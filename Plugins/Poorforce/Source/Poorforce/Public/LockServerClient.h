#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

class IHttpRequest;
class IHttpResponse;

namespace PoorforceLock
{
	struct FLockEntry
	{
		FString OwnerId;
		FString Timestamp;
		FString RawValue;
	};

	enum class EAcquireResult : uint8
	{
		Acquired,
		AlreadyHeld,
		NetworkError,
	};

	FString MakeLockValue(const FString& OwnerId);

	FLockEntry ParseLockValue(const FString& RawValue);
}

class FLockServerClient : public TSharedFromThis<FLockServerClient>
{
public:
	using FAcquireCallback   = TFunction<void(PoorforceLock::EAcquireResult Result)>;
	using FGetCallback       = TFunction<void(bool bExists, const TOptional<PoorforceLock::FLockEntry>& Entry)>;
	using FSimpleCallback    = TFunction<void(bool bSuccess)>;
	using FListOwnedCallback = TFunction<void(bool bSuccess, const TArray<FString>& OwnedKeys)>;

	FLockServerClient(FString InBaseUrl, FString InToken);

	void TryAcquire(const FString& Key, const FString& OwnerId, int32 TtlSeconds, FAcquireCallback OnComplete);
	void Get(const FString& Key, FGetCallback OnComplete);
	void Refresh(const FString& Key, int32 TtlSeconds, FSimpleCallback OnComplete);
	void Release(const FString& Key, FSimpleCallback OnComplete);

	// Redis 를 SCAN + MGET 해서 OwnerId 소유 락 키를 조회한다.
	// 세션 메모리와 무관하게 항상 authoritative (에디터 재시작 후에도 정확).
	void ListOwnedKeys(const FString& MatchPattern, const FString& OwnerId, FListOwnedCallback OnComplete);

private:
	FString BaseUrl;
	FString AuthHeader;

	void SendCommand(
		const TArray<FString>& Args,
		TFunction<void(bool bSuccess, const TSharedPtr<class FJsonValue>& ResultValue)> OnDone);

	// SCAN 커서 루프. 커서 0 도달까지 재귀적으로 키를 Accum 에 모은다.
	void ScanKeys(
		const FString& MatchPattern,
		const FString& Cursor,
		TSharedPtr<TArray<FString>> Accum,
		TFunction<void(bool bSuccess)> OnScanDone);
};
