#include "Character/GYCharacter.h"

#include "AbilitySystemComponent.h"
#include "Logging/GYLogManager.h"

#include "Character/GYPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Player/GYPlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UI/GYUIMessages.h"

AGYCharacter::AGYCharacter()
{
	ActiveEquipmentComponent = CreateDefaultSubobject<UActiveEquipmentComponent>(TEXT("ActiveEquipmentComponent"));
	PawnExtComponent = CreateDefaultSubobject<UGYPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	GetCharacterMovement()->MaxWalkSpeed = 300.f;

}

void AGYCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->InitGAS(this);
	}

	if (PawnExtComponent)
	{
		PawnExtComponent->CheckDefaultInitialization();
	}

	BroadcastCharacterReady();

	if (!HasAuthority()) return;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;
	if (!IsValid(ActiveEquipmentComponent)) return;

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	Loadout->OnLoadoutSlotChanged.AddUObject(
		ActiveEquipmentComponent.Get(),
		&UActiveEquipmentComponent::OnLoadoutSlotChanged);

	for (const FEquipmentLoadoutEntry& Entry : Loadout->GetEntries())
	{
		ActiveEquipmentComponent->OnLoadoutSlotChanged(Entry.SlotTag, Entry.InstanceId);
	}
}

void AGYCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->InitGAS(this);
	}

	if (PawnExtComponent)
	{
		PawnExtComponent->CheckDefaultInitialization();
	}

	BroadcastCharacterReady(); // 컨트롤러가 늦게 복제될 때도 알림
}

void AGYCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (PawnExtComponent)
	{
		PawnExtComponent->CheckDefaultInitialization();
	}

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	PS->InitGAS(this);

	BroadcastCharacterReady();
}



UAbilitySystemComponent* AGYCharacter::GetAbilitySystemComponent() const
{
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

void AGYCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	GY_LOG(Player, KHB, "SetupPlayerInputComponent 호출됨. IC: %s", PlayerInputComponent ? *PlayerInputComponent->GetClass()->GetName() : TEXT("null"));
	InputComponent = PlayerInputComponent;

	if (PawnExtComponent)
	{
		// 멈춰있던 초기화 상태가 있다면 마저 진행하라고 체인을 다시 굴려줍니다.
		PawnExtComponent->CheckDefaultInitialization();
	}
}

void AGYCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	// 1. 매니저에게 이 액터를 리시버(Receiver)로 등록하여 컴포넌트 주입을 받을 수 있게 합니다.
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGYCharacter::BeginPlay()
{
	// 2. 확장을 기다리는 다른 플러그인들에게 이 액터가 게임플레이 준비가 되었다고 알립니다.
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGYCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 3. 액터가 파괴될 때 매니저에서 리시버 등록을 해제합니다.
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGYCharacter::BroadcastCharacterReady()
{
	if (!GetAbilitySystemComponent()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FGYCharacterReadyMessage Msg;
	Msg.OwnerActor = this;

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Character_Ready,Msg);
}
