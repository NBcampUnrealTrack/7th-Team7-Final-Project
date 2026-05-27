#include "Test/GYTestCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "Player/GYPlayerState.h"

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

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->InitTestGAS(this);
	}

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

	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->InitTestGAS(this);
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
			EIC->BindAction(AttackAction, ETriggerEvent::Started,   this, &AGYTestCharacter::OnAttack);
			EIC->BindAction(AttackAction, ETriggerEvent::Completed, this, &AGYTestCharacter::OnAttackReleased);
		}
		if (ParryAction)
		{
			EIC->BindAction(ParryAction, ETriggerEvent::Started, this, &AGYTestCharacter::OnParry);
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
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->HandleAttackInput();
	}
}

void AGYTestCharacter::OnAttackReleased(const FInputActionValue& Value)
{
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->HandleAttackReleasedInput();
	}
}

void AGYTestCharacter::OnParry(const FInputActionValue& Value)
{
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		PS->HandleParryInput();
	}
}
