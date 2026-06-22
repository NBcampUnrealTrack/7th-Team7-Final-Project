#pragma once

#include "Engine/DataAsset.h"
#include "GYExperienceDefinition.generated.h"

class UGYPawnData;

UCLASS(BlueprintType, Const, Meta = (DisplayName = "GY Experience Definition"))
class GY_API UGYExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 이 Experience가 활성화할 게임 피쳐 플러그인 이름 목록 (예: "ComponentInjectionFeature")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GY|Experience")
	TArray<FString> GameFeaturesToEnable;

	// 폰 스폰 시 사용할 PawnData. 현재는 캐릭터 BP 디폴트가 PawnData를 들고 있어 미사용 가능.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GY|Experience")
	TObjectPtr<const UGYPawnData> DefaultPawnData;
};
