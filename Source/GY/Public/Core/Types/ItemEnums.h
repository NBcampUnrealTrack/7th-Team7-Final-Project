#pragma once

#include "CoreMinimal.h"
#include "ItemEnums.generated.h"

/** 인벤토리 이벤트 종류. UInventoryManagerComponent::OnInventoryChanged 인자. */
UENUM(BlueprintType)
enum class EInventoryEventType : uint8
{
	Added,
	Removed,
	StackCountChanged,
	Mutated      // 강화/인챈트/잼/충전 등 항목 내용 변경
};
