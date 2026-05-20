// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/GYCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"
#include "GameFramework/SpringArmComponent.h"

const FName UGYCameraComponent::NAME_ActorFeatureName("CameraComponent");

UGYCameraComponent::UGYCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UGYCameraComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                            FGameplayTag DesiredState) const
{
	check(Manager);
	APawn* Pawn = GetPawn<APawn>();

	// [None] -> Spawned
	if (!CurrentState.IsValid() && DesiredState == GYGameplayTags::InitState_Spawned)
	{
		return IsValid(Pawn);
	}

	// Spawned -> DataAvailable
	if (CurrentState == GYGameplayTags::InitState_Spawned &&
		DesiredState == GYGameplayTags::InitState_DataAvailable)
	{
		return true;
	}

	// DataAvailable -> DataInitialized (PawnExtension이 같은 단계 이상 도달했는가)
	if (CurrentState == GYGameplayTags::InitState_DataAvailable &&
		DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		return Manager->HasFeatureReachedInitState(
			Pawn,
			UGYPawnExtensionComponent::NAME_ActorFeatureName,
			GYGameplayTags::InitState_DataInitialized);
	}

	// DataInitialized -> GameplayReady
	if (CurrentState == GYGameplayTags::InitState_DataInitialized &&
		DesiredState == GYGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UGYCameraComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                               FGameplayTag DesiredState)
{
	if (DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (!Pawn || !Pawn->GetRootComponent()) return;

		// 1. 런타임에 스프링 암 동적 생성(NewObject) 및 세팅
		SpringArmComponent = NewObject<USpringArmComponent>(Pawn, TEXT("SpringArmComponent"));
		if (SpringArmComponent)
		{
			SpringArmComponent->SetupAttachment(Pawn->GetRootComponent());
			SpringArmComponent->TargetArmLength = 800.0f;
			SpringArmComponent->bUsePawnControlRotation = false; // 회전 연동 끄기


			SpringArmComponent->SetUsingAbsoluteRotation(true);
			SpringArmComponent->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
			SpringArmComponent->bDoCollisionTest = false;

			// 캐릭터(Pawn)가 회전할 때 카메라가 따라서 빙글빙글 돌지 않도록 고정합니다.
			SpringArmComponent->bInheritPitch = false;
			SpringArmComponent->bInheritYaw = false;
			SpringArmComponent->bInheritRoll = false;
			// 탑뷰는 천장이나 벽에 카메라가 부딪혀서 화면으로 훅 당겨지는 현상을 보통 끕니다.
			SpringArmComponent->bDoCollisionTest = false;
			SpringArmComponent->RegisterComponent(); // 런타임 생성 컴포넌트는 반드시 수동으로 레지스터 호출 - 오너등록
		}

		// 2. 런타임에 카메라 동적 생성 및 세팅
		CameraComponent = NewObject<UCameraComponent>(Pawn, TEXT("CameraComponent"));
		if (CameraComponent)
		{
			CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
			CameraComponent->bUsePawnControlRotation = false;
			CameraComponent->RegisterComponent();
		}
	}
}

void UGYCameraComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	CheckDefaultInitialization();
}

void UGYCameraComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {
		GYGameplayTags::InitState_Spawned,
		GYGameplayTags::InitState_DataAvailable,
		GYGameplayTags::InitState_DataInitialized,
		GYGameplayTags::InitState_GameplayReady
	};
	ContinueInitStateChain(StateChain);
}

void UGYCameraComponent::OnRegister()
{
	RegisterInitStateFeature();
	Super::OnRegister();
}

void UGYCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	BindOnActorInitStateChanged(UGYPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);
	ensure(TryToChangeInitState(GYGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UGYCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

