#include "Character/GYCharacter.h"

#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "Logging/GYLogManager.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"

#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Character/GYPlayerActionConfig.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Character/Revive/GYReviveConfig.h"
#include "Character/Revive/RevivePoolComponent.h"
#include "Character/Revive/ReviveProgressComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Character/HitReactionComponent.h"
#include "Character/Climbing/ClimbingComponent.h"
#include "GameModes/GYGameMode.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "AbilitySystem/GYOnHitModifierComponent.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTeams/GYTeams.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionOption.h"
#include "Player/GYPlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "UI/GYUIMessages.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"

AGYCharacter::AGYCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UGYCharacterMovementComponent>(
	ACharacter::CharacterMovementComponentName))
{
	ActiveEquipmentComponent = CreateDefaultSubobject<UActiveEquipmentComponent>(TEXT("ActiveEquipmentComponent"));
	OnHitModifierComponent = CreateDefaultSubobject<UGYOnHitModifierComponent>(TEXT("OnHitModifierComponent"));
	PawnExtComponent = CreateDefaultSubobject<UGYPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));
	ClimbingComponent = CreateDefaultSubobject<UClimbingComponent>(TEXT("ClimbingComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	PhysicalAnimationComponent = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));
	HitReactionComponent = CreateDefaultSubobject<UHitReactionComponent>(TEXT("HitReaction"));
	RevivePoolComponent = CreateDefaultSubobject<URevivePoolComponent>(TEXT("RevivePoolComponent"));
	ReviveProgressComponent = CreateDefaultSubobject<UReviveProgressComponent>(TEXT("ReviveProgressComponent"));
	GetCharacterMovement()->MaxWalkSpeed = 300.f;

	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
	StimuliSource->bAutoRegister = true;

	TeamId = FGenericTeamId(GYTeams::Player);
}


void AGYCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// ASC init(InitGAS)은 PawnExtension의 InitState(DataAvailable)로 이관됨.
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

	// 장비 로드아웃 구독·초기 동기화는 컴포넌트가 담당.
	ActiveEquipmentComponent->InitializeLoadoutBinding();
}

void AGYCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

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

void AGYCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYCharacter, bIsDead);
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

void AGYCharacter::Server_StartFacingLerp_Implementation(float StartYaw, float TargetYaw, float LerpTime)
{
	GetWorldTimerManager().ClearTimer(FacingLerpTimer);

	const float Delta = FRotator::NormalizeAxis(TargetYaw - StartYaw);
	if (FMath::IsNearlyZero(Delta)) return;

	if (LerpTime <= 0.f)
	{
		FRotator NewRot = GetActorRotation();
		NewRot.Yaw = TargetYaw;
		SetActorRotation(NewRot);
		return;
	}

	FacingLerpStartYaw = StartYaw;
	FacingLerpTargetYaw = TargetYaw;
	FacingLerpDuration = LerpTime;
	FacingLerpStartTime = GetWorld()->GetTimeSeconds();

	GetWorldTimerManager().SetTimer(FacingLerpTimer, this, &AGYCharacter::TickFacingLerp, 0.016f, true);
}

void AGYCharacter::TickFacingLerp()
{
	const float Elapsed = GetWorld()->GetTimeSeconds() - FacingLerpStartTime;
	const float Alpha = FMath::Clamp(Elapsed / FacingLerpDuration, 0.f, 1.f);
	const float EasedAlpha = 1.f - FMath::Square(1.f - Alpha);
	const float DeltaYaw = FRotator::NormalizeAxis(FacingLerpTargetYaw - FacingLerpStartYaw);

	FRotator NewRot = GetActorRotation();
	NewRot.Yaw = FacingLerpStartYaw + DeltaYaw * EasedAlpha;
	SetActorRotation(NewRot);

	if (Alpha >= 1.f)
	{
		GetWorldTimerManager().ClearTimer(FacingLerpTimer);
	}
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
	// UE_LOG(LogTemp, Warning, TEXT("[Noise] Footstep at %s by %s"),
	// 	*GetActorLocation().ToString(), *GetName());
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		1.f,
		this,
		1500.f,
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

const UGYPlayerActionConfig* AGYCharacter::GetActionConfig() const
{
	if (PawnExtComponent)
	{
		if (const UGYPawnData* PawnData = PawnExtComponent->GetPawnData())
		{
			return PawnData->ActionConfig;
		}
	}
	return nullptr;
}

void AGYCharacter::HandleDeath()
{
	if (!HasAuthority()) return;
	bIsDead = true;

	if (LockOnComponent)
	{
		LockOnComponent->StopLockOn();
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();

		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
		{
			GYASC->RemoveCombatTag();
		}
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	const UGYPlayerActionConfig* Config = GetActionConfig();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	const UGYReviveConfig* ReviveConfig = Config ? Config->ReviveConfig.Get() : nullptr;
	const int32 NumPlayers = GetWorld() ? GetWorld()->GetNumPlayerControllers() : 0;

	if (NumPlayers >= 2 && ReviveConfig)
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Interactable, ECR_Block);
		EnterDownedState(ReviveConfig);
	}
	else
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
		{
			if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
			{
				if (Config)
				{
					for (const FGameplayTag& Tag : Config->DeathTags)
					{
						ASC->Grant_AddLooseTag(GYStateTags::State_Life_Dead, Tag, 1, EGameplayTagReplicationState::TagOnly);
					}
				}
			}
		}

		if (AGYGameMode* GM = GetWorld()->GetAuthGameMode<AGYGameMode>())
		{
			const float Delay = Config ? Config->RespawnDelay : 5.f;
			GM->RequestRespawn(PC, Delay);
		}
	}
}

void AGYCharacter::OnRep_bIsDead()
{
	if (bIsDead)
	{
		EnableRagdoll();
	}
	else
	{
		DisableRagdoll();
		ReactivateGameplayAbilities();
	}
}

void AGYCharacter::ReactivateGameplayAbilities()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
		{
			ASC->InitAbilityActorInfo(PS, this);
		}
	}
}

void AGYCharacter::EnableRagdoll()
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	if (HitReactionComponent)
	{
		HitReactionComponent->StopHitReaction();
	}

	SkeletalMesh->SetCollisionProfileName(TEXT("Ragdoll"));
	SkeletalMesh->SetConstraintProfile(TEXT("pelvis"), TEXT("Ragdoll"));
	SkeletalMesh->SetSimulatePhysics(true);
}

void AGYCharacter::DisableRagdoll()
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	SkeletalMesh->SetAllBodiesSimulatePhysics(false);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->PutAllRigidBodiesToSleep();
	SkeletalMesh->SetAllBodiesPhysicsBlendWeight(0.f);
	SkeletalMesh->bBlendPhysics = false;

	SkeletalMesh->SetCollisionProfileName(TEXT("None"));
	SkeletalMesh->SetConstraintProfile(TEXT("pelvis"), TEXT("None"));

	SkeletalMesh->AttachToComponent(
		GetCapsuleComponent(),
		FAttachmentTransformRules::SnapToTargetIncludingScale);

	if (const AGYCharacter* CDO = GetClass()->GetDefaultObject<AGYCharacter>())
	{
		if (const USkeletalMeshComponent* CDOMesh = CDO->GetMesh())
		{
			SkeletalMesh->SetRelativeLocationAndRotation(
				CDOMesh->GetRelativeLocation(),
				CDOMesh->GetRelativeRotation());
		}
	}

	SkeletalMesh->RecreatePhysicsState();

	if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.f);
	}
	SkeletalMesh->InitAnim(true);

	if (ActiveEquipmentComponent)
	{
		ActiveEquipmentComponent->ReapplyAnimLayers();
	}
}

void AGYCharacter::EnterDownedState(const UGYReviveConfig* Config)
{
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		{
			for (const FGameplayTag& Tag : Config->DownedTags)
			{
				ASC->Grant_AddLooseTag(GYStateTags::State_Life_Downed, Tag, 1, EGameplayTagReplicationState::TagOnly);
			}
		}
	}

	if (RevivePoolComponent)
	{
		RevivePoolComponent->ActivatePool(const_cast<UGYReviveConfig*>(Config), GetActorLocation());
	}

	CheckAndForceGiveUpIfAllDown();
}

void AGYCharacter::CheckAndForceGiveUpIfAllDown()
{
	if (!HasAuthority()) return;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		AGYCharacter* Char = Cast<AGYCharacter>(PC->GetPawn());
		if (Char && !Char->bIsDead) return;
	}

	TArray<AGYCharacter*> DownedChars;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		AGYCharacter* Char = Cast<AGYCharacter>(PC->GetPawn());
		if (Char && Char->IsDowned())
		{
			DownedChars.Add(Char);
		}
	}

	for (AGYCharacter* Char : DownedChars)
	{
		Char->GiveUp();
	}
}

void AGYCharacter::Revive(const UGYReviveConfig* Config)
{
	if (!HasAuthority()) return;

	bIsDead = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Interactable, ECR_Ignore);

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		{
			if (Config)
			{
				ASC->RevokeGrantSource(GYStateTags::State_Life_Downed);

				const float MaxHP = ASC->GetNumericAttribute(UGYPlayerVitalAttributeSet::GetMaxHealthAttribute());
				ASC->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetCurrentHealthAttribute(),
					MaxHP * Config->ReviveHealthGrantedPercent);
			}
		}
	}

	ReactivateGameplayAbilities();
}

void AGYCharacter::StartGiveUpTimer()
{
	Server_StartGiveUp();
}

void AGYCharacter::CancelGiveUpTimer()
{
	Server_CancelGiveUp();
}

void AGYCharacter::Server_StartGiveUp_Implementation()
{
	if (!bIsDead) return;

	const UGYReviveConfig* Config = nullptr;
	if (const UGYPlayerActionConfig* ActionConfig = GetActionConfig())
	{
		Config = ActionConfig->ReviveConfig.Get();
	}

	const float HoldTime = Config ? Config->GiveUpHoldTime : 3.f;
	GetWorldTimerManager().SetTimer(GiveUpTimerHandle, this, &AGYCharacter::GiveUp, HoldTime, false);
}

void AGYCharacter::Server_CancelGiveUp_Implementation()
{
	GetWorldTimerManager().ClearTimer(GiveUpTimerHandle);
}

void AGYCharacter::GiveUp()
{
	if (!HasAuthority()) return;

	if (RevivePoolComponent)
	{
		RevivePoolComponent->DeactivatePool();
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	const UGYPlayerActionConfig* ActionConfig = GetActionConfig();
	const UGYReviveConfig* ReviveConfig = ActionConfig ? ActionConfig->ReviveConfig.Get() : nullptr;

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		{
			if (ReviveConfig)
			{
				ASC->RevokeGrantSource(GYStateTags::State_Life_Downed);
			}

			if (ActionConfig)
			{
				for (const FGameplayTag& Tag : ActionConfig->DeathTags)
				{
					ASC->Grant_AddLooseTag(GYStateTags::State_Life_Dead, Tag, 1, EGameplayTagReplicationState::TagOnly);
				}
			}
		}
	}

	if (AGYGameMode* GM = GetWorld()->GetAuthGameMode<AGYGameMode>())
	{
		const float Delay = ReviveConfig ? ReviveConfig->RespawnDelayAfterGiveUp : 3.f;
		GM->RequestRespawn(PC, Delay);
	}
}

bool AGYCharacter::IsDowned() const
{
	return RevivePoolComponent && RevivePoolComponent->IsPoolActive();
}

const UGYReviveConfig* AGYCharacter::GetReviveConfig() const
{
	const UGYPlayerActionConfig* Config = GetActionConfig();
	return Config ? Config->ReviveConfig.Get() : nullptr;
}

void AGYCharacter::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	if (RevivePoolComponent)
	{
		RevivePoolComponent->AppendInteractionOptions(Interactor, OutOptions);
	}
}

void AGYCharacter::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (RevivePoolComponent)
	{
		RevivePoolComponent->HandleInteract(OptionTag, Interactor);
	}
}
