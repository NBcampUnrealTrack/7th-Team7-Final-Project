#pragma once

#include "CoreMinimal.h"

// =============================
// Categories
// =============================

GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYContent, Log, All);
GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYCombat, Log, All);
GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYAI, Log, All);
GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYUI, Log, All);
GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYPlayer, Log, All);
GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYNetwork, Log, All);
GY_API DECLARE_LOG_CATEGORY_EXTERN(LogGYGame, Log, All);

// =============================
// Config
// =============================

class GY_API FGYLogConfig
{
public:

	static bool IsAllEnabled();

	static bool IsDeveloperEnabled(const FString& DeveloperName);
};

// =============================
// Macros
// =============================

#if !UE_BUILD_SHIPPING

#define GY_INTERNAL_LOG(Category, Developer, Verbosity, Format, ...) \
if (FGYLogConfig::IsAllEnabled() && \
FGYLogConfig::IsDeveloperEnabled(TEXT(#Developer))) \
{ \
UE_LOG(LogGY##Category, Verbosity, TEXT("[%s] ") TEXT(Format), \
TEXT(#Developer), ##__VA_ARGS__); \
}

#define GY_LOG(Category, Developer, Format, ...) \
GY_INTERNAL_LOG(Category, Developer, Log, Format, ##__VA_ARGS__)

#define GY_WARN(Category, Developer, Format, ...) \
GY_INTERNAL_LOG(Category, Developer, Warning, Format, ##__VA_ARGS__)

#define GY_ERROR(Category, Developer, Format, ...) \
GY_INTERNAL_LOG(Category, Developer, Error, Format, ##__VA_ARGS__)

#else

#define GY_LOG(Category, Developer, Format, ...)
#define GY_WARN(Category, Developer, Format, ...)
#define GY_ERROR(Category, Developer, Format, ...)

#endif
