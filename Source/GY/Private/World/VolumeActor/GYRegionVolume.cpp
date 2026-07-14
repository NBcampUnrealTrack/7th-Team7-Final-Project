#include "World/VolumeActor/GYRegionVolume.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Loot/RegionLootData.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "GameStates/GYGameState.h"

void AGYRegionVolume::HandlePawnEntered(APawn* Pawn)
{
	if (!Pawn) return;

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	if (Region->RegionId.IsValid()) // 지역 태그 갱신
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			FGameplayTag RegionParentTag = FGameplayTag::RequestGameplayTag(TEXT("Region"));
			FGameplayTagContainer OwnedTags;

			ASC->GetOwnedGameplayTags(OwnedTags);
			FGameplayTagContainer TagsToRemove = OwnedTags.Filter(FGameplayTagContainer(RegionParentTag));

			ASC->RemoveLooseGameplayTags(TagsToRemove);
			ASC->AddLooseGameplayTag(Region->RegionId);
		}
	}
	if (GetNetMode() == NM_DedicatedServer) return;

	// 로컬 플레이어 본인이 들어온 경우에만 이 지역 BGM으로 전환
	if (Pawn->IsLocallyControlled() && Region->RegionBGM.IsValid())
	{
		if (AGYGameState* GYGameState = GetWorld()->GetGameState<AGYGameState>())
		{
			GYGameState->PlayGameBGMLocal(Region->RegionBGM);
		}
	}

	FGYRegionEnteredMessage Msg;
	Msg.RegionId = Region->RegionId;
	Msg.RegionDisplayName = Region->RegionDisplayName;
	Msg.RegionLevel = GetDefault<UGYWorldDataSettings>()->DefaultRegionLevel;
	Msg.RegionIcon = Region->RegionIcon;
	Msg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Entered, Msg);
}

void AGYRegionVolume::HandlePawnExited(APawn* Pawn)
{
	if (!Pawn) return;

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	// 태그 제거
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		if (!ASC->HasMatchingGameplayTag(Region->RegionId)) return;
		ASC->RemoveLooseGameplayTag(Region->RegionId);
	}

	if (GetNetMode() == NM_DedicatedServer) return;

	FGYRegionExitedMessage ExitMsg;
	ExitMsg.RegionId = Region->RegionId;
	ExitMsg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Exited, ExitMsg);
}
