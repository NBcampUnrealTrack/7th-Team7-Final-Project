#include "Cheats/GYServerCheatProxy.h"

#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Core/GameplayTags/CurrencyTags.h"
#include "Currency/CurrencyComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/LootBoxActor.h"
#include "Player/GYPlayerState.h"

namespace
{
	AGYPlayerState* GetGYPlayerState(const AGYServerCheatProxy* GYServerCheatProxy)
	{
		APlayerController* PC = GYServerCheatProxy->OwnerController.Get();
		if (!IsValid(PC)) return nullptr;
		return PC->GetPlayerState<AGYPlayerState>();
	}

	APawn* GetCheatPawn(const AGYServerCheatProxy* GYServerCheatProxy)
	{
		APlayerController* PC = GYServerCheatProxy->OwnerController.Get();
		return IsValid(PC) ? PC->GetPawn() : nullptr;
	}
}
AGYServerCheatProxy::AGYServerCheatProxy()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	bNetUseOwnerRelevancy = true;
	PrimaryActorTick.bCanEverTick = false;
}

void AGYServerCheatProxy::Server_AddXP_Implementation(float Amount)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!PS) return;
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(this, FName(TEXT("GE_Cheat_AddXP")));
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UGYPlayerAttribute::GetXPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FScalableFloat(Amount);
	Effect->Modifiers.Add(Modifier);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	ASC->ApplyGameplayEffectToSelf(Effect, 1.f, Context);
}

void AGYServerCheatProxy::Server_SpawnEnemy_Implementation(EEnemyType EnemyType)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	const TSoftClassPtr<AGYEnemyCharacterBase>* Found = EnemyClassMap.Find(EnemyType);
	if (!Found || Found->IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_SpawnEnemy: EnemyClassMap에 타입 등록 안 됨"));
		return;
	}

	TSubclassOf<AGYEnemyCharacterBase> EnemyClass = Found->LoadSynchronous();
	if (!EnemyClass) return;

	FVector SpawnLoc = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 300.f;

	// 바닥 LineTrace
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);

	const FVector TraceStart = SpawnLoc + FVector(0.f, 0.f, 500.f);
	const FVector TraceEnd   = SpawnLoc - FVector(0.f, 0.f, 1000.f);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		SpawnLoc = HitResult.ImpactPoint;  // 바닥 표면 위치로 교체
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGYEnemyCharacterBase* Enemy = Pawn->GetWorld()->SpawnActor<AGYEnemyCharacterBase>(
		EnemyClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (IsValid(Enemy))
	{
		Enemy->InitWithType(EnemyType);
	}
}

void AGYServerCheatProxy::Server_KillAllEnemies_Implementation()
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGYEnemyCharacterBase::StaticClass(), Enemies);

	for (AActor* Actor : Enemies)
	{
		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Actor))
		{
			if (!Enemy->IsDead())
			{
				Enemy->Die();
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("GY_KillAllEnemies: %d 마리 처리"), Enemies.Num());
}

void AGYServerCheatProxy::Server_SpawnLootBox_Implementation(const FString& SourceId, const FString& LootTablePath)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	UDataTable* Table = LoadObject<UDataTable>(nullptr, *LootTablePath);
	if (!IsValid(Table))
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_SpawnLootBox: failed to load table %s"), *LootTablePath);
		return;
	}

	const FVector SpawnLocation = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 200.f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ALootBoxActor* Box = Pawn->GetWorld()->SpawnActor<ALootBoxActor>(
		ALootBoxActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);

	if (!IsValid(Box)) return;

	Box->LootSourceId = FName(*SourceId);
	Box->LootTable = Table;

	UE_LOG(LogTemp, Log, TEXT("Server_SpawnLootBox: spawned %s (SourceId=%s)"),
		*Box->GetName(), *SourceId);
}

void AGYServerCheatProxy::Server_InvokeInteraction_Implementation(AActor* Target, FGameplayTag OptionTag)
{
	if (!IsValid(Target)) return;

	IInteractable* Interactable = Cast<IInteractable>(Target);
	if (Interactable == nullptr) return;

	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	Interactable->OnInteract(OptionTag, Pawn);
}

void AGYServerCheatProxy::Server_TakeFromLootBox_Implementation(AActor* Box, int32 DropIndex)
{
	ALootBoxActor* LootBox = Cast<ALootBoxActor>(Box);
	if (!IsValid(LootBox)) return;

	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	LootBox->TakeItem(DropIndex, Pawn);
}

void AGYServerCheatProxy::Server_TakeAllLoot_Implementation(AActor* Box)
{
	ALootBoxActor* LootBox = Cast<ALootBoxActor>(Box);
	if (!IsValid(LootBox)) return;

	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	LootBox->TakeAll(Pawn);
}




void AGYServerCheatProxy::Server_AddItem_Implementation(const FString& ItemPath, int32 Count)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	UItemDefinition* Def = LoadObject<UItemDefinition>(nullptr, *ItemPath);
	if (!IsValid(Def))
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_AddItem: failed to load %s"), *ItemPath);
		return;
	}

	FGuid OutId;
	if (Inv->TryAddItem(Def, Count, OutId))
	{
		UE_LOG(LogTemp, Log, TEXT("Server_AddItem: %s x%d (InstanceId=%s)"),
			*Def->ItemId.ToString(), Count, *OutId.ToString());
	}
}

void AGYServerCheatProxy::Server_EquipItem_Implementation(const FString& ItemPath)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Inv) || !IsValid(Loadout)) return;

	UItemDefinition* Def = LoadObject<UItemDefinition>(nullptr, *ItemPath);
	if (!IsValid(Def))
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_EquipItem: failed to load %s"), *ItemPath);
		return;
	}

	FGuid OutId;
	if (!Inv->TryAddItem(Def, 1, OutId)) return;

	Loadout->Server_RequestEquip(OutId);
}

void AGYServerCheatProxy::Server_UnequipSlot_Implementation(FGameplayTag SlotTag)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	Loadout->Server_RequestUnequip(SlotTag);
}
void AGYServerCheatProxy::Server_AddTimeShards_Implementation(int32 Amount)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UCurrencyComponent* Currency = PS->GetCurrencyComponent();
	if (!IsValid(Currency)) return;

	Currency->TryAdd(GYGameplayTags::Currency_TimeShard, Amount);
}
