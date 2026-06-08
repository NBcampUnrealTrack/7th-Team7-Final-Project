#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ItemTransactionComponent.generated.h"

class IItemContainer;

// 플레이어 본인 아이템 컨테이너 간 mutation(이동 등) 서버 커맨드 허브.
// 무상태 — RPC 라우팅을 위해서만 복제(복제 프로퍼티 없음). 추후 drop/split 등도 여기에 추가.
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class GY_API UItemTransactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemTransactionComponent();

	// 컨테이너를 포인터가 아니라 ID로 받음 → 서버가 "요청한 플레이어 본인의 컨테이너"로만 resolve.
	// 임시 UObject 페이로드(직렬화 불가)와 남의 컨테이너 지목(보안) 문제를 구조적으로 차단.
	UFUNCTION(Server, Reliable)
	void Server_TransferItem(FGameplayTag FromContainerTag, FGuid InstanceId, FGameplayTag ToContainerTag);

private:
	// 소유 액터(PlayerState)의 컴포넌트 중 ContainerTag가 일치하는 IItemContainer 반환
	TScriptInterface<IItemContainer> ResolveContainer(FGameplayTag ContainerTag) const;
};
