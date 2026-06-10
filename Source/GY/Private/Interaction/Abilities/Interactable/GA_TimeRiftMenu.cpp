#include "Interaction/Abilities/Interactable/GA_TimeRiftMenu.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/CameraTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Player/GYPlayerState.h"
#include "WorldGimmick/TimeRift/TimeRift.h"
#include "WorldGimmick/TimeRift/TimeRiftSubsystem.h"
#include "Core/Sound/GYSoundManager.h"
#include "Logging/GYLogManager.h"

UGA_TimeRiftMenu::UGA_TimeRiftMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift);
}

void UGA_TimeRiftMenu::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);


	UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, GYGameplayTags::Event_TimeRift_Exit);
	Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
	Task->ReadyForActivation();

	ATimeRift* TimeRift = Cast<ATimeRift> (GetCurrentSourceObject());
	if (TimeRift)
	{
		FGuid PersistentGuid =  TimeRift->GetPersistentGuid();
		if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
		{
			if (UTimeRiftSubsystem* TimeRiftSubsystem = GameInstance->GetSubsystem<UTimeRiftSubsystem>())
			{
				TimeRiftSubsystem->NotifyVisited(PersistentGuid);
			}
		}
		if (APlayerController* PlayerController = ActorInfo->PlayerController.Get())
		{
			if (AGYPlayerState* PlayerState = PlayerController->GetPlayerState<AGYPlayerState>())
			{
				PlayerState->SetLastCheckpointId(PersistentGuid);
			}
		}

	}
	GY_LOG(Content, CYS, "시간틈 메뉴열기");
	// 카메라 모드 태그
	AGYPlayerState* PS = Cast<AGYPlayerState>(ActorInfo->OwnerActor.Get());
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	ASC->AddLooseGameplayTag(GYGameplayTags::Camera_Mode_ZoomIn, 1, EGameplayTagReplicationState::CountToOwner);

	if (UGYSoundManager* SoundManager = UGYSoundManager::Get(this))
	{
		// 사운드
		SoundManager->PlaySound2D(GYGameplayTags::Sound_Interaction_TimeRift);
	}
}

void UGA_TimeRiftMenu::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	GY_LOG(Content, CYS, "시간틈 메뉴끝");
	// 카메라 모드 태그 제거
	AGYPlayerState* PS = Cast<AGYPlayerState>(CurrentActorInfo->OwnerActor.Get());
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	ASC->RemoveLooseGameplayTag(GYGameplayTags::Camera_Mode_ZoomIn, 1, EGameplayTagReplicationState::CountToOwner);
}
