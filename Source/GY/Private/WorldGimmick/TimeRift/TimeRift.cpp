#include "WorldGimmick/TimeRift/TimeRift.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Player/GYPlayerState.h"
#include "WorldGimmick/TimeRift/TimeRiftSubsystem.h"


ATimeRift::ATimeRift()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SetRootComponent(StaticMeshComponent);

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(StaticMeshComponent);

	InteractTag = GYGameplayTags::Interaction_TimeRift_Sit;
}

void ATimeRift::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (auto* TimeRiftSubsystem = GameInstance->GetSubsystem<UTimeRiftSubsystem>())
		{
			TimeRiftSubsystem->RegisterActor(this);
		}
	}
}

void ATimeRift::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (auto* TimeRiftSubsystem = GameInstance->GetSubsystem<UTimeRiftSubsystem>())
		{
			TimeRiftSubsystem->UnregisterActor(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}


void ATimeRift::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{

	AGYPlayerState* PlayerState = Interactor->GetPlayerState<AGYPlayerState>();
	if (!PlayerState) return;
	UAbilitySystemComponent* AbilitySystemComponent = PlayerState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent || AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Interaction_TimeRift)) return;

	FInteractionOption Option;
	Option.InteractionAbilityToGrant = SitAbilityClass;
	Option.Text = NSLOCTEXT("TimeRift", "Sit", "앉기");
	Option.OptionTag = InteractTag;
	Option.SourceObject = const_cast<ATimeRift*>(this);
	OutOptions.Add(Option);

	RegisterAsCheckpoint(PlayerState);
}

void ATimeRift::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (!HasAuthority()) return;
	if (OptionTag != InteractTag) return;

	AGYPlayerState* PlayerState = Interactor->GetPlayerState<AGYPlayerState>();
	if (!PlayerState) return;
	UAbilitySystemComponent* AbilitySystemComponent = PlayerState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->AddLooseGameplayTag(GYStateTags::State_Interaction_TimeRift, 1, EGameplayTagReplicationState::TagOnly);

	RegisterAsCheckpoint(PlayerState);
}

void ATimeRift::RegisterAsCheckpoint(AGYPlayerState* PlayerState) const
{
	if (!HasAuthority()) return;
	if (!PlayerState) return;
	if (!PersistentGuid.IsValid()) return;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTimeRiftSubsystem* TimeRiftSubsystem = GameInstance->GetSubsystem<UTimeRiftSubsystem>())
		{
			TimeRiftSubsystem->NotifyVisited(PersistentGuid);
		}
	}
}

