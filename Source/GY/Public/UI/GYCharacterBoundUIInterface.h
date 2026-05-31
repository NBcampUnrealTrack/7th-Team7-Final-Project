#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GYCharacterBoundUIInterface.generated.h"

UINTERFACE(MinimalAPI)
class UGYCharacterBoundUI : public UInterface
{
	GENERATED_BODY()
};

/**
 * UWidgetComponent로 부착되는 위젯들 구현용 인터페이스
 * 각 위젯이 owner 캐릭터를 받아서 데이터 필요한거 꺼내씀
 */
class GY_API IGYCharacterBoundUI
{
	GENERATED_BODY()

public:
	virtual void BindToOwnerCharacter(AActor* InCharacter) = 0;
};

