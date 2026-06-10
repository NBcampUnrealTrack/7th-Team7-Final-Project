#include "Loot/LootViewerComponent.h"

#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Core/Sound/GYSoundManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Loot/LootBoxActor.h"
#include "UI/GYUIMessages.h"

ULootViewerComponent::ULootViewerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULootViewerComponent::Client_ShowLootBox_Implementation(ALootBoxActor* Box)
{
	if (!IsValid(Box)) return;

	UWorld* World = GetWorld();
	if (World == nullptr) return;

	FGYLootBoxStateMessage Msg;
	Msg.Box = Box;
	Msg.bOpened = Box->IsOpened();
	Msg.RemainingDrops = Box->GetPendingDrops().Num();
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Loot_ShowBox, Msg);
}

void ULootViewerComponent::Client_PlayLootBoxSound_Implementation(FGameplayTag SoundTag)
{
	if (UGYSoundManager* SoundManager = UGYSoundManager::Get(this))
	{
		SoundManager->PlaySound2D(SoundTag);
	}
}

void ULootViewerComponent::Server_TakeLootItem_Implementation(ALootBoxActor* Box, int32 DropIndex)
{
	if (!IsValid(Box)) return;

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	Box->TakeItem(DropIndex, PS->GetPawn());
}

void ULootViewerComponent::Server_CloseLootBox_Implementation(ALootBoxActor* Box)
{
	if (!IsValid(Box)) return;

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	Box->ReleaseViewer(PS);
}
