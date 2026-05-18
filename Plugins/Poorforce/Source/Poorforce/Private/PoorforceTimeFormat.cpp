#include "PoorforceTimeFormat.h"

#include "Misc/DateTime.h"

namespace PoorforceTimeFormat
{
	FString FormatElapsedSince(int64 UnixSeconds)
	{
		const int64 NowUnix = FDateTime::UtcNow().ToUnixTimestamp();
		const int64 Elapsed = FMath::Max<int64>(0, NowUnix - UnixSeconds);

		if (Elapsed < 60)
		{
			return FString::Printf(TEXT("%lld초 전"), Elapsed);
		}

		if (Elapsed < 3600)
		{
			return FString::Printf(TEXT("%lld분 전"), Elapsed / 60);
		}

		if (Elapsed < 86400)
		{
			const int64 Hours = Elapsed / 3600;
			const int64 Minutes = (Elapsed % 3600) / 60;
			if (Minutes == 0)
			{
				return FString::Printf(TEXT("%lld시간 전"), Hours);
			}
			return FString::Printf(TEXT("%lld시간 %lld분 전"), Hours, Minutes);
		}

		const int64 Days = Elapsed / 86400;
		return FString::Printf(TEXT("%lld일 전"), Days);
	}

	FString FormatElapsedSinceTimestampString(const FString& UnixSecondsString)
	{
		if (UnixSecondsString.IsEmpty()) return FString(TEXT("(시각 정보 없음)"));

		int64 Parsed = 0;
		LexFromString(Parsed, *UnixSecondsString);
		if (Parsed <= 0) return FString(TEXT("(시각 정보 없음)"));

		return FormatElapsedSince(Parsed);
	}

	bool ParseDuration(const FString& Input, int32& OutSeconds)
	{
		int64 Total = 0;
		int64 Current = 0;
		bool bHasDigit = false;
		bool bHasAnyUnit = false;

		for (int32 i = 0; i < Input.Len(); ++i)
		{
			const TCHAR C = Input[i];

			if (FChar::IsWhitespace(C)) continue;

			if (FChar::IsDigit(C))
			{
				Current = Current * 10 + (C - TEXT('0'));
				bHasDigit = true;
				continue;
			}

			if (!bHasDigit) return false;

			int64 Multiplier = 0;
			switch (C)
			{
				case TEXT('s'): case TEXT('S'): Multiplier = 1; break;
				case TEXT('m'): case TEXT('M'): Multiplier = 60; break;
				case TEXT('h'): case TEXT('H'): Multiplier = 3600; break;
				case TEXT('d'): case TEXT('D'): Multiplier = 86400; break;
				default: return false;
			}

			Total += Current * Multiplier;
			Current = 0;
			bHasDigit = false;
			bHasAnyUnit = true;
		}

		if (bHasDigit) return false;
		if (!bHasAnyUnit) return false;
		if (Total <= 0 || Total > MAX_int32) return false;

		OutSeconds = static_cast<int32>(Total);
		return true;
	}
}
