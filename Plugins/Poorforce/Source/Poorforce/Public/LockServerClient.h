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
	using FAcquireCallback = TFunction<void(PoorforceLock::EAcquireResult Result)>;
	using FGetCallback     = TFunction<void(bool bExists, const TOptional<PoorforceLock::FLockEntry>& Entry)>;
	using FSimpleCallback  = TFunction<void(bool bSuccess)>;

	FLockServerClient(FString InBaseUrl, FString InToken);

	void TryAcquire(const FString& Key, const FString& OwnerId, int32 TtlSeconds, FAcquireCallback OnComplete);
	void Get(const FString& Key, FGetCallback OnComplete);
	void Refresh(const FString& Key, int32 TtlSeconds, FSimpleCallback OnComplete);
	void Release(const FString& Key, FSimpleCallback OnComplete);

private:
	FString BaseUrl;
	FString AuthHeader;

	void SendCommand(
		const TArray<FString>& Args,
		TFunction<void(bool bSuccess, const TSharedPtr<class FJsonValue>& ResultValue)> OnDone);
};
