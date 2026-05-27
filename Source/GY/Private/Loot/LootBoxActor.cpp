#include "Loot/LootBoxActor.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Engine/GameInstance.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Loot/LootService.h"
#include "Loot/RegionLootData.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"

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
}

void ALootBoxActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	if (bOpened && PendingDrops.IsEmpty()) return;

	FInteractionOption Option;
	Option.OptionTag = GYGameplayTags::Interaction_Open_LootBox;
	Option.Text = NSLOCTEXT("LootBox", "Open", "열기");
	OutOptions.Add(Option);
}

void ALootBoxActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (!HasAuthority()) return;
	if (OptionTag != GYGameplayTags::Interaction_Open_LootBox) return;

	if (!bOpened)
	{
		OpenBox(Interactor);
	}
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

	if (PendingDrops.IsEmpty())
	{
		Destroy();
	}
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
}

void ALootBoxActor::OnRep_Opened()
{
}
