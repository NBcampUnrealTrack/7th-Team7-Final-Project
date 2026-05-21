// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYHeroComponent.h"

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
const FName UGYHeroComponent::NAME_ActorFeatureName("Hero");


UGYHeroComponent::UGYHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGYHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                         FGameplayTag DesiredState) const
{
	// 여기서 다음 상태로 넘어갈 조건이 충족되었는지 검사합니다.
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return false;

	if (DesiredState == GYGameplayTags::InitState_Spawned)
	{
		return true;
	}

	if (CurrentState == GYGameplayTags::InitState_Spawned &&
		DesiredState == GYGameplayTags::InitState_DataAvailable)
	{


		if (!GetPlayerState<AGYPlayerState>())
		{
			return false;
		}
		//If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		//번역 : "만약 우리가 권한(Authority)을 가지고 있거나 자율(Autonomous) 상태라면,
		//플레이어 스테이트(Player State)의 소유권이 등록된 컨트롤러를 기다려야 합니다."
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

	if (CurrentState == GYGameplayTags::InitState_DataAvailable &&
		DesiredState== GYGameplayTags::InitState_DataInitialized)
	{
		AGYPlayerState* GYPlayerState = GetPlayerState<AGYPlayerState>();

		if (Pawn->IsLocallyControlled() && !Pawn->InputComponent)
		{
			return false;
		}
		return GYPlayerState &&
			Manager->HasFeatureReachedInitState(
			Pawn,
			UGYPawnExtensionComponent::NAME_ActorFeatureName,
			GYGameplayTags::InitState_DataInitialized);
	}

	if (CurrentState == GYGameplayTags::InitState_DataInitialized &&
		DesiredState ==GYGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UGYHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	GY_LOG(Player, KHB, "HeroComp : [%s] -> [%s]", *CurrentState.ToString(), *DesiredState.ToString());

	// 내가 DataInitialized 단계에 무사히 진입했다면(즉, PawnExtension도 준비가 끝났다면) 입력을 세팅합니다.
	if (DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AGYPlayerState* GYPlayerState = GetPlayerState<AGYPlayerState>();
		if (!Pawn || !GYPlayerState) return;

		if (UInputComponent* PlayerInputComponent = Pawn->InputComponent)
		{
			GY_LOG(Player, KHB, "InitializePlayerInput 호출. IC 클래스: %s", *PlayerInputComponent->GetClass()->GetName());
			InitializePlayerInput(PlayerInputComponent);
		}
		else
		{
			GY_ERROR(Player, KHB, "Pawn->InputComponent가 null - 입력 초기화 스킵됨 -서버");
		}
	}
}

void UGYHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UGYPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == GYGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UGYHeroComponent::CheckDefaultInitialization()
{
	GY_LOG(Player, KHB, "HeroComp: CheckDefaultInitialization 호출됨");
	// 초기화 체인 굴리기 시작
	static const TArray<FGameplayTag> StateChain = {
		GYGameplayTags::InitState_Spawned,
		GYGameplayTags::InitState_DataAvailable,
		GYGameplayTags::InitState_DataInitialized,
		GYGameplayTags::InitState_GameplayReady
	};
	ContinueInitStateChain(StateChain);
}

void UGYHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	UGYPawnExtensionComponent* ExtComp = Pawn->FindComponentByClass<UGYPawnExtensionComponent>();
	check(ExtComp);

	const UGYPawnData* PawnData = ExtComp->PawnData;
	if (!PawnData)
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

void UGYHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	GY_LOG(Player, KHB, "Input_Move 호출됨");
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

void UGYHeroComponent::OnRegister()
{
	RegisterInitStateFeature();
	Super::OnRegister();
}



// Called when the game starts
void UGYHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	BindOnActorInitStateChanged(UGYPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);
	ensure(TryToChangeInitState(GYGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();

}

void UGYHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}


