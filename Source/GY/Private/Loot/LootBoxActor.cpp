#include "Loot/LootBoxActor.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "WorldGimmick/DoorMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTags/CameraTags.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Core/GameplayTags/QuestTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"
#include "Loot/LootService.h"
#include "Loot/LootViewerComponent.h"
#include "Loot/RegionLootData.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "World/VolumeActor/GYRegionVolume.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Items/ItemDefinition.h"

ALootBoxActor::ALootBoxActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Mesh->SetCollisionProfileName(GYCollisionProfile::Interactable);
}

void ALootBoxActor::BeginPlay()
{
	Super::BeginPlay();

	GetComponents<UDoorMovementComponent>(MovementComponent);

	if (!HasAuthority()) return;

	// 스폰 확률 — 실패하면 등장하지 않음
	if (SpawnChance < 1.f && FMath::FRand() > SpawnChance)
	{
		Destroy();
		return;
	}

	// 박스에 직접 RegionData를 지정하지 않았으면, 자신이 속한 지역 볼륨에서 상속
	if (RegionData.IsNull())
	{
		TArray<AActor*> Volumes;
		UGameplayStatics::GetAllActorsOfClass(this, AGYRegionVolume::StaticClass(), Volumes);
		for (AActor* Actor : Volumes)
		{
			AGYRegionVolume* Volume = Cast<AGYRegionVolume>(Actor);
			if (IsValid(Volume) && Volume->IsLocationInside(GetActorLocation()) && !Volume->GetRegionData().IsNull())
			{
				RegionData = Volume->GetRegionData();
				break;
			}
		}
	}
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
		const bool bIsNewViewer = (CurrentViewer != Interactor->GetPlayerState());
		CurrentViewer = Interactor->GetPlayerState();
		ShowToInteractor(Interactor);
		if (bIsNewViewer)
		{
			// 열림
			PlayOpenEffect(Interactor);
		}
		else
		{
			// 닫힘
			PlayCloseEffect(Interactor);
		}
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

	if (APawn* Pawn = Viewer->GetPawn())
	{
		PlayCloseEffect(Pawn);
	}

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
	OnRep_Opened(); // 서버는 OnRep가 자동 호출되지 않으므로 직접 호출

	FGYLootBoxStateMessage OpenMsg;
	OpenMsg.Box = this;
	OpenMsg.bOpened = true;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(GYGameplayTags::Message_Loot_BoxOpened, OpenMsg);

	PlayFirstEffect(Opener);
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
	const int32 Added = Inv->TryAddItem(Drop.Definition, Drop.Count, OutId, [&Drop](FInventoryEntry& Entry)
	{
		Entry.GradeTag = Drop.GradeTag;
		Entry.Level = Drop.Level;
		Entry.StatDeviation = Drop.StatDeviation;
		Entry.RolledOptions = Drop.RolledOptions;
		Entry.EnhancementLevel = Drop.EnhancementLevel;
		Entry.RandomSeed = Drop.UsedSeed;
	});
	if (Added <= 0) return; // 가방이 꽉 차 못 넣음 — 상자에 그대로 유지

	PS->Client_PlaySound(GYGameplayTags::Sound_Item_Looting);

	const UItemDefinition* ItemDef = Drop.Definition.LoadSynchronous();
	if (ItemDef && ItemDef->FindFragment<UItemFragment_Weapon>())
	{
		// 무기 획득 퀘스트 - 003 활성
		UWorld* World = GetWorld();
		if (IsValid(World))
		{
			FQuestEventMessage QuestMsg;
			QuestMsg.EventTag = GYGameplayTags::Quest_Activate_ItemObtain;
			QuestMsg.TargetId = "Weapon";
			QuestMsg.Count = 1;
			UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Quest_Event, QuestMsg);
		}
	}

	if (Added >= Drop.Count)
	{
		PendingDrops.RemoveAt(DropIndex);
	}
	else
	{
		// 일부만 들어감 — 남은 수량은 상자에 유지
		PendingDrops[DropIndex].Count -= Added;
	}

	// 빈 상자라도 파괴하지 않음 — 직접 닫기 전까지 유지, 다른 플레이어가 빈 것을 확인 가능
	// authority(리슨서버/호스트/데디 서버 모두)는 OnRep이 안 뜨므로 직접 통지
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
	if (bOpened)
	{
		for (UDoorMovementComponent* MovementComp : MovementComponent)
		{
			MovementComp->SetOpen(true);
			GY_LOG(Content, CYS, "열려라 참깨");
		}
	}
	BroadcastStateChanged();
}

void ALootBoxActor::BroadcastStateChanged()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;

	// 루트 박스 열기 퀘스트 - 001 목표 / 002 활성
	FQuestEventMessage QuestMsg;
	QuestMsg.EventTag = GYGameplayTags::Quest_Objective_OpenLootBox;
	QuestMsg.Count = 1;
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Quest_Event, QuestMsg);

	if (World->IsNetMode(NM_DedicatedServer)) return;

	FGYLootBoxStateMessage Msg;
	Msg.Box = this;
	Msg.bOpened = bOpened;
	Msg.RemainingDrops = PendingDrops.Num();
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Loot_BoxStateChanged, Msg);
}

void ALootBoxActor::PlayOpenEffect(APawn* Opener)
{
	if (!IsValid(Opener)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Opener);
	if (!ASC) return;

	ASC->AddLooseGameplayTag(GYGameplayTags::Camera_Mode_ZoomIn, 1, EGameplayTagReplicationState::CountToOwner);

	if (AGYPlayerState* PS = Opener->GetPlayerState<AGYPlayerState>())
	{
		if (ULootViewerComponent* Viewer = PS->GetLootViewerComponent())
		{
			Viewer->Client_PlayLootBoxSound(GYGameplayTags::Sound_Interaction_LootBox_Open);
		}
	}
}

void ALootBoxActor::PlayCloseEffect(APawn* Opener)
{
	if (!IsValid(Opener)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Opener);
	if (!ASC) return;

	const int32 Count = ASC->GetGameplayTagCount(GYGameplayTags::Camera_Mode_ZoomIn);
	if (Count > 0)
	{
		ASC->RemoveLooseGameplayTag(GYGameplayTags::Camera_Mode_ZoomIn, Count,
		                            EGameplayTagReplicationState::CountToOwner);
	}

	if (AGYPlayerState* PS = Opener->GetPlayerState<AGYPlayerState>())
	{
		if (ULootViewerComponent* LootViewer = PS->GetLootViewerComponent())
		{
			LootViewer->Client_PlayLootBoxSound(GYGameplayTags::Sound_Interaction_LootBox_Close);
		}
	}
}

void ALootBoxActor::PlayFirstEffect(APawn* Opener)
{
	if (!IsValid(Opener)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Opener);
	if (!ASC) return;
	FGameplayCueParameters Parameters;
	Parameters.Location = GetActorLocation();
	Parameters.Normal=GetActorForwardVector();
	ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Interaction_LootBox, Parameters);
}
