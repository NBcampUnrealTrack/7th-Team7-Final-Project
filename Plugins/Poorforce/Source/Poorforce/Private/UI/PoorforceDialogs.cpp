#include "UI/PoorforceDialogs.h"

#include "PoorforceLog.h"
#include "RcloneProcessManager.h"
#include "UI/SUploadProgressWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

namespace
{
	TSharedRef<SWindow> MakeHostWindow(const FText& Title, FVector2D Size)
	{
		return SNew(SWindow)
			.Title(Title)
			.ClientSize(Size)
			.SizingRule(ESizingRule::FixedSize)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.HasCloseButton(true);
	}

	TSharedPtr<SWindow> GetParentWindowForModal()
	{
		if (FSlateApplication::IsInitialized())
		{
			TSharedPtr<SWindow> ActiveTop = FSlateApplication::Get().GetActiveTopLevelWindow();
			if (ActiveTop.IsValid()) return ActiveTop;
		}
		return TSharedPtr<SWindow>{};
	}
}

namespace PoorforceDialogs
{
	EBlockedDialogResult ShowBlockedDialog(
		const FString& RelativePath,
		const FString& OwnerId,
		const FString& ElapsedText,
		bool bShowForceUnlock,
		bool bShowOpenAnyway)
	{
		EBlockedDialogResult Result = EBlockedDialogResult::Confirmed;

		const TSharedRef<SWindow> Host = MakeHostWindow(
			FText::FromString(TEXT("Poorforce — 잠긴 에셋")),
			FVector2D(560.f, 260.f));

		Host->SetContent(
			SNew(SBlockedDialog)
			.RelativePath(RelativePath)
			.OwnerId(OwnerId)
			.ElapsedText(ElapsedText)
			.ParentWindow(Host)
			.OutResult(&Result)
			.bShowForceUnlock(bShowForceUnlock)
			.bShowOpenAnyway(bShowOpenAnyway));

		FSlateApplication::Get().AddModalWindow(Host, GetParentWindowForModal(), /*bSlowTaskWindow=*/ false);

		return Result;
	}

	FForceUnlockDialogResult ShowForceUnlockDialog(
		const FString& RelativePath,
		const FString& OwnerId)
	{
		FForceUnlockDialogResult Result;

		const TSharedRef<SWindow> Host = MakeHostWindow(
			FText::FromString(TEXT("Poorforce — 강제 해제")),
			FVector2D(480.f, 300.f));

		Host->SetContent(
			SNew(SForceUnlockDialog)
			.RelativePath(RelativePath)
			.OwnerId(OwnerId)
			.ParentWindow(Host)
			.OutResult(&Result));

		FSlateApplication::Get().AddModalWindow(Host, GetParentWindowForModal(), /*bSlowTaskWindow=*/ false);

		return Result;
	}

	EUploadRetryChoice ShowUploadRetryDialog(
		const FString& RelativePath,
		int32 ExitCode,
		const FString& Output)
	{
		EUploadRetryChoice Choice = EUploadRetryChoice::Dismiss;

		const TSharedRef<SWindow> Host = MakeHostWindow(
			FText::FromString(TEXT("Poorforce — 업로드 실패")),
			FVector2D(560.f, 420.f));

		Host->SetContent(
			SNew(SUploadRetryDialog)
			.RelativePath(RelativePath)
			.ExitCode(ExitCode)
			.Output(Output)
			.ParentWindow(Host)
			.OutChoice(&Choice));

		FSlateApplication::Get().AddModalWindow(Host, GetParentWindowForModal(), /*bSlowTaskWindow=*/ false);

		return Choice;
	}

	bool WaitForUploadsModal(TSharedPtr<FRcloneProcessManager> Rclone)
	{
		if (!Rclone.IsValid() || !Rclone->HasActive()) return false;

		UE_LOG(LogPoorforce, Log, TEXT("Editor exit delayed — %d rclone process(es) still running"), Rclone->NumActive());

		bool bForceCancelled = false;

		const TSharedRef<SWindow> Host = MakeHostWindow(
			FText::FromString(TEXT("Poorforce — 업로드 진행 중")),
			FVector2D(560.f, 320.f));

		Host->SetContent(
			SNew(SUploadProgressWidget)
			.Rclone(Rclone)
			.ParentWindow(Host)
			.OutForceCancelled(&bForceCancelled));

		FSlateApplication::Get().AddModalWindow(Host, GetParentWindowForModal(), /*bSlowTaskWindow=*/ false);

		if (bForceCancelled)
		{
			UE_LOG(LogPoorforce, Warning, TEXT("User force-quit during upload — cancelling %d active process(es). Locks may remain (TTL will release)."),
				Rclone->NumActive());
			Rclone->CancelAll();
		}

		return bForceCancelled;
	}
}
