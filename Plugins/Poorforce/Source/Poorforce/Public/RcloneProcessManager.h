#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"

class FRcloneProcessManager
{
public:
	enum class EDirection : uint8
	{
		Download,
		Upload,
	};

	using FOnComplete = TFunction<void(bool bSuccess, int32 ExitCode, const FString& Output)>;

	FRcloneProcessManager(FString InExecutable);
	~FRcloneProcessManager();

	void StartCopyTo(
		EDirection Direction,
		const FString& LocalPath,
		const FString& RemotePath,
		FOnComplete OnComplete);

	// rclone check — 비교만, transfer 절대 안 함.
	// bSuccess=true: 두 파일 일치 / bSuccess=false: 불일치 (또는 네트워크/셋업 오류)
	void StartCheck(
		const FString& LocalPath,
		const FString& RemotePath,
		FOnComplete OnComplete);

	int32 NumActive() const { return Active.Num(); }
	bool HasActive() const { return Active.Num() > 0; }

	TArray<FString> GetActiveDescriptions() const;

	void CancelAll();

private:
	struct FActiveProcess
	{
		FProcHandle Handle;
		void* ReadPipe = nullptr;
		void* WritePipe = nullptr;
		FString AccumulatedOutput;
		FOnComplete OnComplete;
		FString Description;
	};

	FString Executable;
	TArray<TUniquePtr<FActiveProcess>> Active;
	FTSTicker::FDelegateHandle TickerHandle;

	bool Tick(float DeltaTime);

	void FinalizeProcess(FActiveProcess& Proc, bool bSuccess, int32 ExitCode);
};
