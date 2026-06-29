#pragma once

#include "CoreMinimal.h"

// 세이브 data JSON 의 섹션 키 상수
// 컴포넌트의 GetSaveSectionKey / GetRestoreDependencies 가 같은 상수를 참조
namespace GYSaveSectionKeys
{
	inline const FString Currency  = TEXT("currency");
	inline const FString Inventory = TEXT("inventory");
	inline const FString Equipment = TEXT("equipment");
	inline const FString Stats     = TEXT("stats");
	inline const FString SkillTree = TEXT("skilltree");
}
