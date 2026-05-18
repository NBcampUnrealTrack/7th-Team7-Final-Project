// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYHeroComponet.h"

#include "EnhancedInputSubsystems.h"
#include "Character/GYInputComponent.h"
#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/InputTag.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"
#include "GameFramework/PlayerState.h"
#include "Logging/GYLogManager.h"
#include "Player/GYPlayerState.h"

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
	// 여기서 다음 상태로 넘어갈 조건이 충족되었는지 검사합니다.
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (DesiredState == GYGameplayTags::InitState_Spawned)
	{
		if (Pawn)
		{
			return true;
		}
	}

	if (CurrentState == GYGameplayTags::InitState_Spawned && DesiredState == GYGameplayTags::InitState_DataAvailable)
	{


		if (!GetPlayerState<AGYPlayerState>())
		{
			return false;
		}
		//If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == GYGameplayTags::InitState_DataAvailable && DesiredState ==
		GYGameplayTags::InitState_DataInitialized)
	{
		return Manager->HasFeatureReachedInitState(Pawn, UGYPawnExtensionComponent::NAME_ActorFeatureName,
												   GYGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == GYGameplayTags::InitState_DataInitialized && DesiredState ==
		GYGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UGYHeroComponet::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	// 내가 DataInitialized 단계에 무사히 진입했다면(즉, PawnExtension도 준비가 끝났다면) 입력을 세팅합니다.
	if (DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (!Pawn) return;
		if (UInputComponent* PlayerInputComponent = Pawn->InputComponent)
		{
			InitializePlayerInput(PlayerInputComponent);
		}
	}
}

void UGYHeroComponet::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	CheckDefaultInitialization();
}

void UGYHeroComponet::CheckDefaultInitialization()
{
	// 초기화 체인 굴리기 시작
	static const TArray<FGameplayTag> StateChain = {
		GYGameplayTags::InitState_Spawned,
		GYGameplayTags::InitState_DataAvailable,
		GYGameplayTags::InitState_DataInitialized,
		GYGameplayTags::InitState_GameplayReady
	};
	ContinueInitStateChain(StateChain);
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
	RegisterInitStateFeature();
	Super::OnRegister();
}



// Called when the game starts
void UGYHeroComponet::BeginPlay()
{
	Super::BeginPlay();

	BindOnActorInitStateChanged(UGYPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);
	ensure(TryToChangeInitState(GYGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();

}

void UGYHeroComponet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}


