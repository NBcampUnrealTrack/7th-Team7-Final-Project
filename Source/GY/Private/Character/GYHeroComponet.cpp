// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYHeroComponet.h"

#include "EnhancedInputSubsystems.h"
#include "Character/GYInputComponent.h"
#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Core/GameplayTags/InputTag.h"
#include "Logging/GYLogManager.h"
// 이 컴포넌트의 이름표는 "Hero"로 지정합니다.
const FName UGYHeroComponet::NAME_ActorFeatureName("Hero");


UGYHeroComponet::UGYHeroComponet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGYHeroComponet::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                         FGameplayTag DesiredState) const
{
	return IGameFrameworkInitStateInterface::CanChangeInitState(Manager, CurrentState, DesiredState);
}

void UGYHeroComponet::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	IGameFrameworkInitStateInterface::HandleChangeInitState(Manager, CurrentState, DesiredState);
}

void UGYHeroComponet::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	IGameFrameworkInitStateInterface::OnActorInitStateChanged(Params);
}

void UGYHeroComponet::CheckDefaultInitialization()
{
	IGameFrameworkInitStateInterface::CheckDefaultInitialization();
}

void UGYHeroComponet::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	UGYPawnExtensionComponent* ExtComp = Pawn->FindComponentByClass<UGYPawnExtensionComponent>();
	check(ExtComp);

	const UGYPawnData* PawnData = ExtComp->PawnData;
	if (!ExtComp->PawnData)
	{
		GY_WARN(Player, KHB, "PawnData가 할당되지 않았습니다.")
		return;
	}

	//이 클래스가 부착된 컨트롤러 가져오기

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!PlayerController) return;

	//IMC 설정
	if (UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem =
		PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		if (PawnData->DefaultIMC)
		{
			EnhancedInputSubsystem->AddMappingContext(ExtComp->PawnData->DefaultIMC, 0);
		}
	}
	//커스텀 입력 컴포넌트로 캐스팅 후 태그 기반 바인딩
	if (UGYInputComponent* InputComponent = Cast<UGYInputComponent>(PlayerInputComponent))
	{

		InputComponent->BindNativeAction(PawnData->InputConfig, GYGameplayTags::InputTag_Move, ETriggerEvent::Triggered,
										 this, &ThisClass::Input_Move, true);
	}
	else
	{
		GY_ERROR(Player, KHB, "InputComponent가 GYINputComponent가 아님");
	}

}

void UGYHeroComponet::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UGYHeroComponet::OnRegister()
{
	Super::OnRegister();
}

void UGYHeroComponet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


// Called when the game starts
void UGYHeroComponet::BeginPlay()
{
	Super::BeginPlay();

	// ...

}



