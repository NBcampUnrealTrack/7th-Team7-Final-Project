#pragma once

#include "CoreMinimal.h"

namespace PoorforceTimeFormat
{
	FString FormatElapsedSince(int64 UnixSeconds);

	FString FormatElapsedSinceTimestampString(const FString& UnixSecondsString);

	bool ParseDuration(const FString& Input, int32& OutSeconds);
}
