#include "WorldGimmick/DoorActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/BoxComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameStates/GYGameState.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GYUIMessages.h"

ADoorActor::ADoorActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(Scene);

	DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	DoorFrameMesh->SetupAttachment(Scene);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetCollisionProfileName(GYCollisionProfile::Interactable);
	InteractionBox->SetupAttachment(Scene);
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();

	GetComponents<UDoorMovementComponent>(DoorComponents);
}

void ADoorActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOption) const
{
	FInteractionOption Option;
	Option.OptionTag = GYGameplayTags::Interaction_Open_Door;
	Option.Text = bIsOpen ? NSLOCTEXT("Door", "Close", "닫기") : NSLOCTEXT("Door", "Open", "열기");
    Option.SourceObject = const_cast<ADoorActor*>(this);
	OutOption.Add(Option);
}

void ADoorActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (HasAuthority() == false)
	{
		return;
	}
	if (QuestRequiredTag.IsValid())
	{
		AGYGameState* GameState = Cast<AGYGameState>(UGameplayStatics::GetGameState(this));
		if (!GameState) return;

		if (!GameState->IsQuestComplete(QuestRequiredTag))
		{
			//TODO Locked UI
			return;
		}
	}

	DoorMove();
	if (bIsOpen)
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

bool ADoorActor::GetDoorState() const
{
	return bIsOpen;
}

void ADoorActor::DoorMove()
{
	if (HasAuthority() == false)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("InitializeTimeline Function Called!"));

	bIsOpen = !bIsOpen;

	OnRep_Open();
}

void ADoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorActor, bIsOpen)
}

void ADoorActor::OnRep_Open()
{
	for (UDoorMovementComponent* DoorComp : DoorComponents)
	{
		DoorComp->SetOpen(bIsOpen);
	}
	// 볼륨 안에 그대로 있어도 위젯 텍스트가 바로 갱신
	if (GetNetMode() != NM_DedicatedServer)
	{
		FGYInteractionOptionsMessage Msg;
		GatherInteractionOptions(nullptr, Msg.Options);

		UGameplayMessageSubsystem::Get(this).BroadcastMessage(
			GYGameplayTags::Message_Interaction_OptionsChanged, Msg);
	}
}

void ADoorActor::PlayOpenEffect(APawn* Interactor)
{
	GY_LOG(Content, CYS, "문 열림 이펙트");
	if (!IsValid(Interactor)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Interactor);
	if (!ASC) return;

	FGameplayCueParameters CueParameters;
	CueParameters.Location = GetActorLocation();
	CueParameters.Normal = GetActorForwardVector();
	ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Interaction_Door_Open, CueParameters);
	ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Camera_Shake, CueParameters);
}

void ADoorActor::PlayCloseEffect(APawn* Interactor)
{
	GY_LOG(Content, CYS, "문 닫힘 이펙트");
	if (!IsValid(Interactor)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Interactor);
	if (!ASC) return;

	FGameplayCueParameters CueParameters;
	CueParameters.Location = GetActorLocation();
	ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Interaction_Door_Close, CueParameters);
}
