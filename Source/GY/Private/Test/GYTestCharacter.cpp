#include "Test/GYTestCharacter.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Abilities/GameplayAbility.h"

AGYTestCharacter::AGYTestCharacter()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.f;
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

UAbilitySystemComponent* AGYTestCharacter::GetAbilitySystemComponent() const
{
	if (const AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

void AGYTestCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitGAS();

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
}

void AGYTestCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (const AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(const_cast<AGYPlayerState*>(PS), this);
	}
}

void AGYTestCharacter::InitGAS()
{
	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(PS, this);

	ASC->SetNumericAttributeBase(UGYBaseAttribute::GetCurrentHealthAttribute(), InitialHealth);
	ASC->SetNumericAttributeBase(UGYBaseAttribute::GetMaxHealthAttribute(), InitialHealth);

	if (TestAbilitySet)
	{
		TestAbilitySet->GiveToAbilitySystem(ASC, &AbilitySetHandles);
	}

}

void AGYTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGYTestCharacter::OnMove);
		}
		if (AttackAction)
		{
			EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AGYTestCharacter::OnAttack);
			EIC->BindAction(AttackAction, ETriggerEvent::Completed, this, &AGYTestCharacter::OnAttackReleased);
		}
	}
}

void AGYTestCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
	}
}

void AGYTestCharacter::OnAttack(const FInputActionValue& Value)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability &&
			Spec.Ability->AbilityTags.HasTag(GYGameplayTags::Ability_Attack_Charge))
		{
			return;
		}
	}

	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(GYGameplayTags::Ability_Attack_Combo));

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Input_Attack;
	Payload.Instigator = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, GYGameplayTags::Event_Input_Attack, Payload);

	GetWorld()->GetTimerManager().SetTimer(
		HoldToChargeTimer,
		this,
		&AGYTestCharacter::OnHoldToChargeThreshold,
		HoldToChargeTime,
		false
	);
}

void AGYTestCharacter::OnAttackReleased(const FInputActionValue& Value)
{
	GetWorld()->GetTimerManager().ClearTimer(HoldToChargeTimer);

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Input_AttackRelease;
	Payload.Instigator = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, GYGameplayTags::Event_Input_AttackRelease, Payload);
}

void AGYTestCharacter::OnHoldToChargeThreshold()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	FGameplayEventData CancelPayload;
	CancelPayload.EventTag = GYGameplayTags::Event_Input_AttackCharge;
	CancelPayload.Instigator = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, GYGameplayTags::Event_Input_AttackCharge, CancelPayload);

	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(GYGameplayTags::Ability_Attack_Charge));
}
