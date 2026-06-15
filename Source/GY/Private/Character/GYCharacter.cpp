#include "Character/GYCharacter.h"

#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "Logging/GYLogManager.h"

#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Character/GYPlayerActionConfig.h"
#include "Character/LockOn/LockOnComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "Character/HitReactionComponent.h"
#include "Character/Climbing/ClimbingComponent.h"
#include "GameModes/GYGameMode.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Player/GYPlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "UI/GYUIMessages.h"

AGYCharacter::AGYCharacter()
{
	ActiveEquipmentComponent = CreateDefaultSubobject<UActiveEquipmentComponent>(TEXT("ActiveEquipmentComponent"));
	PawnExtComponent = CreateDefaultSubobject<UGYPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));
	ClimbingComponent = CreateDefaultSubobject<UClimbingComponent>(TEXT("ClimbingComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	PhysicalAnimationComponent = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));
	HitReactionComponent = CreateDefaultSubobject<UHitReactionComponent>(TEXT("HitReaction"));
	GetCharacterMovement()->MaxWalkSpeed = 300.f;

	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
	StimuliSource->bAutoRegister = true;
}

void AGYCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->InitGAS(this);
	}

	SubscribeHealthDelegate();

	if (PawnExtComponent)
	{
		PawnExtComponent->CheckDefaultInitialization();
	}

	BroadcastCharacterReady();

	if (!HasAuthority()) return;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;
	if (!IsValid(ActiveEquipmentComponent)) return;

	ensureMsgf(LockOnComponent, TEXT("LockOnComponent Is Null"));
	LockOnComponent->BindToASC(PS);
	ensureMsgf(ClimbingComponent, TEXT("ClimbingComponent Is Null"));
	ClimbingComponent->BindToASC(PS);


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

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();

	ensureMsgf(LockOnComponent, TEXT("LockOnComponent Is Null"));
	LockOnComponent->BindToASC(PS);
	ensureMsgf(ClimbingComponent, TEXT("ClimbingComponent Is Null"));
	ClimbingComponent->BindToASC(PS);

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

	ensureMsgf(LockOnComponent, TEXT("LockOnComponent Is Null"));
	LockOnComponent->BindToASC(PS);
	ensureMsgf(ClimbingComponent, TEXT("ClimbingComponent Is Null"));
	ClimbingComponent->BindToASC(PS);

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

void AGYCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();

		if (!ASC) return;

		if (GetCharacterMovement()->IsFalling())
		{

			ASC->AddLooseGameplayTag(GYStateTags::State_Falling);
		}
		else
		{

			ASC->RemoveLooseGameplayTag(GYStateTags::State_Falling);
		}
	}
}

void AGYCharacter::Server_SetFacingYaw_Implementation(float Yaw)
{
	FRotator NewRot = GetActorRotation();
	NewRot.Yaw = Yaw;
	SetActorRotation(NewRot);
}

void AGYCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	GY_LOG(Player, KHB, "SetupPlayerInputComponent 호출됨. IC: %s",
	       PlayerInputComponent ? *PlayerInputComponent->GetClass()->GetName() : TEXT("null"));
	InputComponent = PlayerInputComponent;

	if (PawnExtComponent)
	{
		// 멈춰있던 초기화 상태가 있다면 마저 진행하라고 체인을 다시 굴려줍니다.
		PawnExtComponent->CheckDefaultInitialization();
	}
}

void AGYCharacter::MakeFootstepNoise()
{
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		0.5f,
		this,
		800.f,
		FName("Footstep")
	);
}

void AGYCharacter::MakeSkillNoise(float Loudness, float MaxRange)
{
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		Loudness,
		this,
		MaxRange,
		FName("Skill")
	);
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
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		this, UGameFrameworkComponentManager::NAME_GameActorReady);
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

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Character_Ready, Msg);
}

void AGYCharacter::SubscribeHealthDelegate()
{
	if (bHealthDelegateBound) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	ASC->GetGameplayAttributeValueChangeDelegate(
		UGYPlayerVitalAttributeSet::GetCurrentHealthAttribute())
		.AddUObject(this, &AGYCharacter::OnHealthChanged);

	bHealthDelegateBound = true;
}

void AGYCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && !bIsDead)
	{
		HandleDeath();
	}
}

void AGYCharacter::HandleDeath()
{
	if (!HasAuthority()) return;
	bIsDead = true;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const UGYPlayerActionConfig* Config = nullptr;
	if (PawnExtComponent && PawnExtComponent->PawnData)
	{
		Config = PawnExtComponent->PawnData->ActionConfig;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (Config)
		{
			for (const FGameplayTag& Tag : Config->DeathTags)
			{
				ASC->AddLooseGameplayTag(Tag, 1, EGameplayTagReplicationState::TagOnly);
			}
		}
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (AGYGameMode* GM = GetWorld()->GetAuthGameMode<AGYGameMode>())
	{
		const float Delay = Config ? Config->RespawnDelay : 5.f;
		GM->RequestRespawn(PC, Delay);
	}
}
