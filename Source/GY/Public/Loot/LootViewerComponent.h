#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootViewerComponent.generated.h"

class ALootBoxActor;

// 플레이어 소유 컴포넌트 — 루트박스 UI 세션의 클라↔서버 RPC 중계점.
// LootBoxActor는 월드 액터라 클라 소유가 아니어서 클라↔서버 RPC를 호스팅할 수 없으므로
// 줍기/닫기 요청과 opener 표시 RPC를 이 컴포넌트가 대신 라우팅한다.
UCLASS(ClassGroup = (Loot), meta = (BlueprintSpawnableComponent))
class GY_API ULootViewerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootViewerComponent();

	// [SERVER→OWNER CLIENT] opener에게만 UI 표시. LootBoxActor가 호출
	UFUNCTION(Client, Reliable)
	void Client_ShowLootBox(ALootBoxActor* Box);

	// [CLIENT→SERVER] 드롭 슬롯 클릭 — 단건 줍기
	UFUNCTION(Server, Reliable)
	void Server_TakeLootItem(ALootBoxActor* Box, int32 DropIndex);

	// [CLIENT→SERVER] UI 닫힘 — 점유 해제
	UFUNCTION(Server, Reliable)
	void Server_CloseLootBox(ALootBoxActor* Box);
};
