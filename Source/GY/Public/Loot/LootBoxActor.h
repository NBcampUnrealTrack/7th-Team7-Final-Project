#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Loot/LootTypes.h"
#include "LootBoxActor.generated.h"

class APlayerState;
class URegionLootData;
class UStaticMeshComponent;

UCLASS()
class GY_API ALootBoxActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALootBoxActor();

	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 비워두면 BeginPlay에 자신이 속한 AGYRegionVolume의 RegionData를 자동 상속
	UPROPERTY(EditAnywhere, Category = "Loot")
	TSoftObjectPtr<URegionLootData> RegionData;

	// 스폰 확률. 1 미만이면 서버 BeginPlay에서 굴려 실패 시 박스가 등장하지 않음
	UPROPERTY(EditAnywhere, Category = "Loot", meta = (ClampMin = 0, ClampMax = 1))
	float SpawnChance = 1.f;

	void OpenBox(APawn* Opener);

	void TakeItem(int32 DropIndex, APawn* Taker);

	void TakeAll(APawn* Taker);

	// 점유 해제 — 점유자가 UI를 닫을 때 PC가 호출
	void ReleaseViewer(APlayerState* Viewer);

	const TArray<FLootDrop>& GetPendingDrops() const { return PendingDrops; }
	bool IsOpened() const { return bOpened; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_PendingDrops();

	UFUNCTION()
	void OnRep_Opened();

	UPROPERTY(ReplicatedUsing = OnRep_PendingDrops, VisibleInstanceOnly, Category = "Loot")
	TArray<FLootDrop> PendingDrops;

	UPROPERTY(ReplicatedUsing = OnRep_Opened, VisibleInstanceOnly, Category = "Loot")
	bool bOpened = false;

	// 현재 UI로 박스를 점유한 플레이어. 유효하면 타인은 열기 불가
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Loot")
	TObjectPtr<APlayerState> CurrentViewer;

private:
	// 점유자 본인이 아니면 true (열기 차단 조건)
	bool IsOccupiedByOther(APawn* Interactor) const;

	// opener 클라이언트로 UI 표시 요청
	void ShowToInteractor(APawn* Interactor);

	// 갱신 GMS 브로드캐스트 (데디 서버 제외)
	void BroadcastStateChanged();
};
