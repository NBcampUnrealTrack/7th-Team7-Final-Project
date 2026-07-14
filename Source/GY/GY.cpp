#include "GY.h"
#include "Misc/NetworkVersion.h"
#include "Modules/ModuleManager.h"

// 클라(런처 엔진)와 데디(커스텀 소스 엔진)의 빌드 스탬프(Changelist) 차이로
// 핸드셰이크가 OutdatedClient 로 거부됨 — 프로젝트 고정 버전으로 통일.
// 리플리케이션/프로토콜이 깨지는 변경을 배포할 때 이 값을 올려서 구버전 접속을 차단한다
constexpr uint32 GYNetworkVersion = 1;

class FGYGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FNetworkVersion::GetLocalNetworkVersionOverride.BindLambda([]() -> uint32 { return GYNetworkVersion; });
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FGYGameModule, GY, "GY");
