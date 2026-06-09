#include "Logging/GYLogManager.h"
#include "HAL/IConsoleManager.h"

// =============================
// Categories
// =============================

DEFINE_LOG_CATEGORY(LogGYContent);
DEFINE_LOG_CATEGORY(LogGYCombat);
DEFINE_LOG_CATEGORY(LogGYAI);
DEFINE_LOG_CATEGORY(LogGYUI);
DEFINE_LOG_CATEGORY(LogGYPlayer);
DEFINE_LOG_CATEGORY(LogGYNetwork);
DEFINE_LOG_CATEGORY(LogGYGame);

// =============================
// Console Variables
// =============================

static TAutoConsoleVariable<int32> CVarGYLog(
	TEXT("gy.log"),
	1,
	TEXT("Enable/Disable all GY logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_KSH(
	TEXT("gy.log.KSH"),
	1,
	TEXT("Enable/Disable KSH logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_KHB(
	TEXT("gy.log.KHB"),
	1,
	TEXT("Enable/Disable KHB logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_KDY(
	TEXT("gy.log.KDY"),
	1,
	TEXT("Enable/Disable KDY logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_LGS(
	TEXT("gy.log.LGS"),
	1,
	TEXT("Enable/Disable LGS logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_JCM(
	TEXT("gy.log.JCM"),
	1,
	TEXT("Enable/Disable JCM logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_CYS(
	TEXT("gy.log.CYS"),
	1,
	TEXT("Enable/Disable CYS logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_KH(
	TEXT("gy.log.KH"),
	1,
	TEXT("Enable/Disable KH logs")
);

static TAutoConsoleVariable<int32> CVarGYLog_ESK(
	TEXT("gy.log.ESK"),
	1,
	TEXT("Enable/Disable ESK logs")
);

// =============================
// Master Toggle
// =============================

bool FGYLogConfig::IsAllEnabled()
{
	return CVarGYLog.GetValueOnGameThread() != 0;
}

// =============================
// Developer Toggle
// =============================

bool FGYLogConfig::IsDeveloperEnabled(const FString& DeveloperName)
{
	IConsoleVariable* Var =
		IConsoleManager::Get().FindConsoleVariable(
			*FString::Printf(TEXT("gy.log.%s"), *DeveloperName)
		);

	if (!Var)
	{
		return true;
	}

	return Var->GetInt() != 0;
}
