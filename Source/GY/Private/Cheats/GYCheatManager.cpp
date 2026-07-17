#include "Cheats/GYCheatManager.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Core/GameplayTags/CurrencyTags.h"
#include "Currency/CurrencyComponent.h"
#include "Currency/CurrencyEntry.h"
#include "Enemy/GYEnemyAIController.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Cheats/GYServerCheatProxy.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionOption.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Interaction/AltarStorageComponent.h"
#include "Player/GYPlayerController.h"
#include "Player/GYPlayerState.h"
#include "Quest/QuestSubsystem.h"

namespace
{
	AGYPlayerState* GetGYPlayerState(const UCheatManager* CheatManager)
	{
		APlayerController* PC = CheatManager->GetOuterAPlayerController();
		if (!IsValid(PC)) return nullptr;
		return PC->GetPlayerState<AGYPlayerState>();
	}

	AGYPlayerController* GetGYPlayerController(const UCheatManager* CheatManager)
	{
		APlayerController* PC = CheatManager->GetOuterAPlayerController();
		if (!IsValid(PC)) return nullptr;
		return Cast<AGYPlayerController>(PC);
	}

	APawn* GetCheatPawn(const UCheatManager* CheatManager)
	{
		APlayerController* PC = CheatManager->GetOuterAPlayerController();
		return IsValid(PC) ? PC->GetPawn() : nullptr;
	}

	AActor* FindNearestInteractable(const UWorld* World, const FVector& Origin, float MaxDistance = 1000.f)
	{
		if (!IsValid(World)) return nullptr;

		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsWithInterface(World, UInteractable::StaticClass(), AllActors);

		AActor* Nearest = nullptr;
		float NearestDistSq = MaxDistance * MaxDistance;

		for (AActor* Actor : AllActors)
		{
			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), Origin);
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Actor;
			}
		}
		return Nearest;
	}

	ALootBoxActor* FindNearestLootBox(const UWorld* World, const FVector& Origin, float MaxDistance = 1000.f)
	{
		if (!IsValid(World)) return nullptr;

		TArray<AActor*> AllBoxes;
		UGameplayStatics::GetAllActorsOfClass(World, ALootBoxActor::StaticClass(), AllBoxes);

		ALootBoxActor* Nearest = nullptr;
		float NearestDistSq = MaxDistance * MaxDistance;

		for (AActor* Actor : AllBoxes)
		{
			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), Origin);
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Cast<ALootBoxActor>(Actor);
			}
		}
		return Nearest;
	}
}

void UGYCheatManager::GY_AddItem(const FString& ItemPath, int32 Count)
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_AddItem(ItemPath, Count);
}

void UGYCheatManager::GY_EquipItem(const FString& ItemPath)
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_EquipItem(ItemPath);
}

void UGYCheatManager::GY_UnequipSlot(const FString& SlotTagName)
{
	const FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*SlotTagName), false);
	if (!SlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_UnequipSlot: invalid tag '%s'"), *SlotTagName);
		return;
	}


	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_UnequipSlot(SlotTag);
}

void UGYCheatManager::GY_PrintInventory()
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	UE_LOG(LogTemp, Log, TEXT("=== Inventory ==="));
	int32 Index = 0;
	for (const FInventoryEntry& Entry : Inv->GetEntries())
	{
		UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		const FName ItemId = IsValid(Def) ? Def->ItemId : NAME_None;

		FString OptionList;
		for (const FRolledEnchantOption& Option : Entry.RolledOptions)
		{
			if (!OptionList.IsEmpty()) OptionList += TEXT(",");
			OptionList += Option.OptionId.ToString();

			FString MagList;
			for (const FRolledMagnitude& Magnitude : Option.Magnitudes)
			{
				if (!MagList.IsEmpty()) MagList += TEXT(" ");
				MagList += FString::Printf(TEXT("%s=%.2f"), *Magnitude.MagnitudeTag.ToString(), Magnitude.Value);
			}
			if (!MagList.IsEmpty()) OptionList += FString::Printf(TEXT("(%s)"), *MagList);
		}
		if (OptionList.IsEmpty()) OptionList = TEXT("-");

		UE_LOG(LogTemp, Log, TEXT("  [%d] %s x%d Lv%d Grade=%s Options=[%s] Dev=%.3f (InstanceId=%s)"),
			Index,
			*ItemId.ToString(),
			Entry.StackCount,
			Entry.Level,
			*Entry.GradeTag.ToString(),
			*OptionList,
			Entry.StatDeviation,
			*Entry.InstanceId.ToString());
		++Index;
	}
}

void UGYCheatManager::GY_PrintLoadout()
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	UE_LOG(LogTemp, Log, TEXT("=== Loadout ==="));
	for (const FEquipmentLoadoutEntry& Entry : Loadout->GetEntries())
	{
		UE_LOG(LogTemp, Log, TEXT("  %s -> %s"),
			*Entry.SlotTag.ToString(), *Entry.InstanceId.ToString());
	}
}


// ============================================================
// Currency / Enchant / Disassemble
// ============================================================

void UGYCheatManager::GY_AddTimeShards(int32 Amount)
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_AddTimeShards(Amount);
}

void UGYCheatManager::GY_PrintCurrency()
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UCurrencyComponent* Currency = PS->GetCurrencyComponent();
	if (!IsValid(Currency)) return;

	UE_LOG(LogTemp, Log, TEXT("=== Currency ==="));
	for (const FCurrencyEntry& Entry : Currency->GetEntries())
	{
		UE_LOG(LogTemp, Log, TEXT("  %s = %d"), *Entry.CurrencyTag.ToString(), Entry.Amount);
	}
}

void UGYCheatManager::GY_Disassemble()
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UAltarStorageComponent* AltarStorage = PS->GetAltarStorageComponent();
	if (!IsValid(AltarStorage)) return;

	AltarStorage->Server_RequestDisassemble();
}

void UGYCheatManager::GY_Enchant(int32 InvIndex)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	const TArray<FInventoryEntry>& Entries = Inv->GetEntries();
	if (!Entries.IsValidIndex(InvIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_Enchant: invalid index %d (inventory size = %d)"), InvIndex, Entries.Num());
		return;
	}

	const FGuid InstanceId = Entries[InvIndex].InstanceId;
	Inv->Server_RequestEnchant(InstanceId);
}



// ============================================================
// Loot / Interaction
// ============================================================

void UGYCheatManager::GY_SpawnLootBox(const FString& RegionDataPath)
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_SpawnLootBox(RegionDataPath);
}

void UGYCheatManager::GY_GetNearestInteractionOptions()
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	AActor* Nearest = FindNearestInteractable(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Nearest))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_GetNearestInteractionOptions: no interactable nearby"));
		return;
	}

	IInteractable* Interactable = Cast<IInteractable>(Nearest);
	if (Interactable == nullptr) return;

	TArray<FInteractionOption> Options;
	Interactable->GatherInteractionOptions(Pawn, Options);

	UE_LOG(LogTemp, Log, TEXT("=== Nearest Interactable: %s ==="), *Nearest->GetName());
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s (Tag=%s)"),
			i, *Options[i].Text.ToString(), *Options[i].OptionTag.ToString());
	}
}

void UGYCheatManager::GY_InvokeInteraction(const FString& OptionTagName)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	const FGameplayTag OptionTag = FGameplayTag::RequestGameplayTag(FName(*OptionTagName), false);
	if (!OptionTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_InvokeInteraction: invalid tag '%s'"), *OptionTagName);
		return;
	}

	AActor* Nearest = FindNearestInteractable(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Nearest))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_InvokeInteraction: no interactable nearby"));
		return;
	}

	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_InvokeInteraction(Nearest, OptionTag);
}

void UGYCheatManager::GY_PrintLootBoxContents()
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	ALootBoxActor* Box = FindNearestLootBox(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Box))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_PrintLootBoxContents: no loot box nearby"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("=== LootBox %s (Opened=%s) ==="),
		*Box->GetName(), Box->IsOpened() ? TEXT("true") : TEXT("false"));

	int32 Index = 0;
	for (const FLootDrop& Drop : Box->GetPendingDrops())
	{
		UItemDefinition* Def = Drop.Definition.LoadSynchronous();
		const FName ItemId = IsValid(Def) ? Def->ItemId : NAME_None;
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s x%d Lv%d (Grade=%s, Dev=%.3f)"),
			Index, *ItemId.ToString(), Drop.Count, Drop.Level,
			*Drop.GradeTag.ToString(), Drop.StatDeviation);
		++Index;
	}
}

void UGYCheatManager::GY_TakeFromLootBox(int32 DropIndex)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	ALootBoxActor* Box = FindNearestLootBox(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Box))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_TakeFromLootBox: no loot box nearby"));
		return;
	}

	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_TakeFromLootBox(Box, DropIndex);
}

void UGYCheatManager::GY_TakeAllLoot()
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	ALootBoxActor* Box = FindNearestLootBox(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Box))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_TakeAllLoot: no loot box nearby"));
		return;
	}

	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_TakeAllLoot(Box);
}

void UGYCheatManager::GY_AddCameraTag(const FString& TagName)
{
	APawn* Pawn = GetCheatPawn(this);

	if (!IsValid(Pawn))
	{
		return;
	}

	IAbilitySystemInterface* ASI =
		Cast<IAbilitySystemInterface>(Pawn);

	if (!ASI)
	{
		return;
	}

	UGYAbilitySystemComponent* ASC =
		Cast<UGYAbilitySystemComponent>(
			ASI->GetAbilitySystemComponent());

	if (!ASC)
	{
		return;
	}

	const FGameplayTag Tag =
		FGameplayTag::RequestGameplayTag(
			FName(*TagName),
			false);

	if (!Tag.IsValid())
	{
		GY_WARN(Player, CYS, "Invalid Tag: %s", *TagName);

		return;
	}

	ASC->AddLooseGameplayTag(Tag);

	GY_LOG(Player, CYS, "Added Camera Tag: %s", *Tag.ToString());
}

void UGYCheatManager::GY_RemoveCameraTag(const FString& TagName)
{
	APawn* Pawn = GetCheatPawn(this);

	if (!IsValid(Pawn))
	{
		return;
	}

	IAbilitySystemInterface* ASI =
		Cast<IAbilitySystemInterface>(Pawn);

	if (!ASI)
	{
		return;
	}

	UGYAbilitySystemComponent* ASC =
		Cast<UGYAbilitySystemComponent>(
			ASI->GetAbilitySystemComponent());

	if (!ASC)
	{
		return;
	}

	const FGameplayTag Tag =
		FGameplayTag::RequestGameplayTag(
			FName(*TagName),
			false);

	if (!Tag.IsValid())
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(Tag);

	GY_LOG(Player, CYS, "Removed Camera Tag: %s", *Tag.ToString());
}

void UGYCheatManager::GY_ToggleCameraTag(const FString& TagName)
{
	APawn* Pawn = GetCheatPawn(this);

	if (!IsValid(Pawn))
	{
		return;
	}

	IAbilitySystemInterface* ASI =
		Cast<IAbilitySystemInterface>(Pawn);

	if (!ASI)
	{
		return;
	}

	UGYAbilitySystemComponent* ASC =
		Cast<UGYAbilitySystemComponent>(
			ASI->GetAbilitySystemComponent());

	if (!ASC)
	{
		return;
	}

	const FGameplayTag Tag =
		FGameplayTag::RequestGameplayTag(
			FName(*TagName),
			false);

	if (!Tag.IsValid())
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(Tag))
	{
		ASC->RemoveLooseGameplayTag(Tag);

		GY_LOG(Player, CYS, "Removed Camera Tag: %s", *Tag.ToString());
	}
	else
	{
		ASC->AddLooseGameplayTag(Tag);

		GY_LOG(Player, CYS, "Added Camera Tag: %s", *Tag.ToString());
	}
}

void UGYCheatManager::GY_QuestComplete(const FString& TagName)
{
	const FGameplayTag Tag =
		FGameplayTag::RequestGameplayTag(
			FName(*TagName));
	if (UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (UQuestSubsystem* QS=GI->GetSubsystem<UQuestSubsystem>())
			{
				QS->StartQuest(Tag);
				QS->CompleteQuest(Tag);
			}
		}
	}
}

void UGYCheatManager::GY_SpawnEnemy(const FString& EnemyTypeName)
{
	EEnemyType Type = EEnemyType::None;
	if (EnemyTypeName.Equals(TEXT("Melee"),  ESearchCase::IgnoreCase)) Type = EEnemyType::Melee;
	else if (EnemyTypeName.Equals(TEXT("Ranged"), ESearchCase::IgnoreCase)) Type = EEnemyType::Ranged;
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_SpawnEnemy: 알 수 없는 타입 '%s'. Melee / Ranged / Boss 중 선택"), *EnemyTypeName);
		return;
	}
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_SpawnEnemy(Type);
}

void UGYCheatManager::GY_KillAllEnemies()
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_KillAllEnemies();
}

void UGYCheatManager::GY_SetEnemyBB(const FString& KeyName, bool bValue)
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGYEnemyCharacterBase::StaticClass(), Enemies);

	for (AActor* Actor : Enemies)
	{
		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Actor))
		{
			if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(Enemy->GetController()))
			{
				if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
				{
					BB->SetValueAsBool(FName(*KeyName), bValue);
				}
			}
		}
	}
}
UE_DISABLE_OPTIMIZATION
void UGYCheatManager::GY_AddXP(float Amount)
{

	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_AddXP(Amount);
}
UE_ENABLE_OPTIMIZATION

void UGYCheatManager::GY_TestHitCue()
{
	APawn* Pawn = GetCheatPawn(this);

	if (!Pawn)
	{
		return;
	}

	IAbilitySystemInterface* ASI =
		Cast<IAbilitySystemInterface>(Pawn);

	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent* ASC =
		ASI->GetAbilitySystemComponent();

	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters Params;

	// 피격 방향
	Params.Normal =
		-Pawn->GetActorForwardVector();

	// 강도
	Params.RawMagnitude = 80.f;

	// 위치
	Params.Location =
		Pawn->GetActorLocation();

	ASC->ExecuteGameplayCue(
		GYGameplayTags::GameplayCue_Camera_Push,
		Params);
}

void UGYCheatManager::GY_Suicide()
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_Suicide();
}

void UGYCheatManager::GY_Invulnerable_Toggle()
{
	AGYPlayerController* AGYPlayerController = GetGYPlayerController(this);
	if (!AGYPlayerController) return;
	TObjectPtr<AGYServerCheatProxy> AGYServerCheatProxy = AGYPlayerController->ServerCheatProxy;
	if (!AGYServerCheatProxy) return;
	AGYServerCheatProxy->Server_Invulnerable_Toggle();
}

