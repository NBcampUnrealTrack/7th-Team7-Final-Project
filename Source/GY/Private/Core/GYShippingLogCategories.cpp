// Shipping 서버 로그 유지(bUseLoggingInShipping)용 링크 보정.
// installed build 엔진의 Shipping 라이브러리는 로깅이 꺼진 채 빌드돼 엔진 로그 카테고리
// 전역 심볼이 없다 — 로깅 켜고 컴파일된 우리 모듈이 (인라인 엔진 헤더 경유로) 참조하는
// 카테고리들을 여기서 대신 정의한다. 모놀리식 링크라 정의 위치는 무관.

#include "CoreMinimal.h"

#if UE_BUILD_SHIPPING && !NO_LOGGING

#include "Engine/CurveTable.h"
#include "DataTableUtils.h"
#include "GameFeaturesSubsystem.h"
#include "JsonGlobals.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogTemp);
DEFINE_LOG_CATEGORY(LogSerialization);
DEFINE_LOG_CATEGORY(LogNetFastTArray);
DEFINE_LOG_CATEGORY(LogJson);
DEFINE_LOG_CATEGORY(LogCurveTable);
DEFINE_LOG_CATEGORY(LogDataTable);
DEFINE_LOG_CATEGORY(LogGameFeatures);
DEFINE_LOG_CATEGORY(LogType);

#endif
