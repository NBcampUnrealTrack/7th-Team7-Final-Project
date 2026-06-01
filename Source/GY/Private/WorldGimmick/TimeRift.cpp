#include "WorldGimmick/TimeRift.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Player/GYPlayerState.h"


ATimeRift::ATimeRift()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SetRootComponent(StaticMeshComponent);

	InteractTag = GYGameplayTags::Interaction_TimeRift_Sit;
}

void ATimeRift::BeginPlay()
{
	Super::BeginPlay();

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
	OutOptions.Add(Option);
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
}

