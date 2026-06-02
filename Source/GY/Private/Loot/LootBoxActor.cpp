#include "Loot/LootBoxActor.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Loot/LootService.h"
#include "Loot/LootViewerComponent.h"
#include "Loot/RegionLootData.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

ALootBoxActor::ALootBoxActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void ALootBoxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALootBoxActor, PendingDrops);
	DOREPLIFETIME(ALootBoxActor, bOpened);
	DOREPLIFETIME(ALootBoxActor, CurrentViewer);
}

void ALootBoxActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	// 다른 플레이어가 점유 중이면 열기 옵션 자체를 숨김 (빈 상자도 열어 확인 가능하므로 비었다고 숨기지 않음)
	if (IsOccupiedByOther(Interactor)) return;

	FInteractionOption Option;
	Option.OptionTag = GYGameplayTags::Interaction_Open_LootBox;
	Option.Text = NSLOCTEXT("LootBox", "Open", "열기");
	OutOptions.Add(Option);
}

void ALootBoxActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (!HasAuthority()) return;
	if (OptionTag != GYGameplayTags::Interaction_Open_LootBox) return;
	if (!IsValid(Interactor)) return;

	// 점유 잠금 — 타인이 보고 있는 박스는 무시
	if (IsOccupiedByOther(Interactor)) return;

	if (!bOpened)
	{
		OpenBox(Interactor);
	}

	// 비어 있어도 점유 + UI 표시 (빈 그리드 확인)
	if (bOpened)
	{
		CurrentViewer = Interactor->GetPlayerState();
		ShowToInteractor(Interactor);
	}
}

bool ALootBoxActor::IsOccupiedByOther(APawn* Interactor) const
{
	if (!IsValid(CurrentViewer)) return false;

	APlayerState* InteractorPS = IsValid(Interactor) ? Interactor->GetPlayerState() : nullptr;
	return CurrentViewer != InteractorPS;
}

void ALootBoxActor::ShowToInteractor(APawn* Interactor)
{
	if (!IsValid(Interactor)) return;

	AGYPlayerState* PS = Interactor->GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	ULootViewerComponent* Viewer = PS->GetLootViewerComponent();
	if (!IsValid(Viewer)) return;

	Viewer->Client_ShowLootBox(this);
}

void ALootBoxActor::ReleaseViewer(APlayerState* Viewer)
{
	if (!HasAuthority()) return;
	if (CurrentViewer != Viewer) return;

	CurrentViewer = nullptr;
	// 빈 상자도 파괴하지 않고 월드에 유지 — 다른 플레이어가 열면 빈 그리드를 봄
}

void ALootBoxActor::OpenBox(APawn* Opener)
{
	if (!HasAuthority()) return;
	if (bOpened) return;

	UGameInstance* GI = GetGameInstance();
	if (!IsValid(GI)) return;

	ULootService* LootService = GI->GetSubsystem<ULootService>();
	if (!IsValid(LootService)) return;

	const URegionLootData* Region = RegionData.LoadSynchronous();
	if (!IsValid(Region)) return;

	FLootContext Context;

	const FRandomStream Seed(FMath::Rand());
	const FLootResult Result = LootService->RollLoot(Region, Context, Seed);

	PendingDrops = Result.Drops;
	bOpened = true;
	// 갱신 알림은 클라의 OnRep에서 처리 (데디 서버는 UI 없음)
}

void ALootBoxActor::TakeItem(int32 DropIndex, APawn* Taker)
{
	if (!HasAuthority()) return;
	if (!PendingDrops.IsValidIndex(DropIndex)) return;
	if (!IsValid(Taker)) return;

	AGYPlayerState* PS = Taker->GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	const FLootDrop& Drop = PendingDrops[DropIndex];

	FGuid OutId;
	if (!Inv->TryAddItem(Drop.Definition, Drop.Count, OutId)) return;

	Inv->MutateEntry(OutId, [&Drop](FInventoryEntry& Entry)
	{
		Entry.GradeTag = Drop.GradeTag;
		Entry.Level = Drop.Level;
		Entry.StatDeviation = Drop.StatDeviation;
		Entry.RolledOptions = Drop.RolledOptions;
		Entry.EnhancementLevel = Drop.EnhancementLevel;
		Entry.RandomSeed = Drop.UsedSeed;
	});

	PendingDrops.RemoveAt(DropIndex);

	// 빈 상자라도 파괴하지 않음 — 직접 닫기 전까지 유지, 다른 플레이어가 빈 것을 확인 가능
	// authority(리슨서버/호스트)는 OnRep이 안 뜨므로 직접 통지. dedicated server는 self-guard로 no-op
	BroadcastStateChanged();
}

void ALootBoxActor::TakeAll(APawn* Taker)
{
	if (!HasAuthority()) return;

	for (int32 i = PendingDrops.Num() - 1; i >= 0; --i)
	{
		TakeItem(i, Taker);
	}
}

void ALootBoxActor::OnRep_PendingDrops()
{
	BroadcastStateChanged();
}

void ALootBoxActor::OnRep_Opened()
{
	BroadcastStateChanged();
}

void ALootBoxActor::BroadcastStateChanged()
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->IsNetMode(NM_DedicatedServer)) return;

	FGYLootBoxStateMessage Msg;
	Msg.Box = this;
	Msg.bOpened = bOpened;
	Msg.RemainingDrops = PendingDrops.Num();
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Loot_BoxStateChanged, Msg);
}
