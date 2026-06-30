// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYHeroComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Character/GYInputComponent.h"
#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Character/GYCharacter.h"
#include "Character/GYPlayerActionConfig.h"
#include "Character/Revive/GYReviveConfig.h"
#include "Components/GameFrameworkComponentManager.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AttackLogic/Charge/GYChargeFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/InputTag.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"
#include "GameFramework/PlayerState.h"
#include "Logging/GYLogManager.h"
#include "Player/GYPlayerState.h"
#include "TimerManager.h"
#include "Player/GYPlayerController.h"

// 이 컴포넌트의 이름표는 "Hero"로 지정합니다.
const FName UGYHeroComponent::NAME_ActorFeatureName("Hero");


UGYHeroComponent::UGYHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	ChargeThresholdEventTags.AddTag(GYGameplayTags::InputTag_Charge);
	ChargeAbilityTags.AddTag(GYGameplayTags::Ability_Attack_Charge);
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
			GY_LOG(Player, KHB, "HeroComp [%s] : 플레이어스테이트 없음.", *CurrentState.ToString());
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
				GY_LOG(Player, KHB, "HeroComp [%s] : 컨트롤러 업슴.", *CurrentState.ToString());
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
			GY_LOG(Player, KHB, "HeroComp [%s] : 인풋컴포넌트없음.", *CurrentState.ToString());
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
	// AbilitySet 부여/ASC 초기화는 PawnExtension이 전담한다.
	if (DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AGYPlayerState* GYPlayerState = GetPlayerState<AGYPlayerState>();
		if (!Pawn || !GYPlayerState) return;

		if (!Pawn->IsLocallyControlled()) return;

		if (UInputComponent* PlayerInputComponent = Pawn->InputComponent)
		{
			GY_LOG(Player, KHB, "InitializePlayerInput 호출. IC 클래스: %s", *PlayerInputComponent->GetClass()->GetName());
			InitializePlayerInput(PlayerInputComponent);
		}
		else
		{
			GY_ERROR(Player, KHB, "Pawn->InputComponent가 null - 입력 초기화 스킵됨");
		}
	}
}

void UGYHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UGYPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == GYGameplayTags::InitState_DataInitialized)
		{
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

	const UGYPawnData* PawnData = ExtComp->GetPawnData();
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
			EnhancedInputSubsystem->AddMappingContext(PawnData->DefaultIMC, 0);
		}
	}
	//커스텀 입력 컴포넌트로 캐스팅 후 태그 기반 바인딩
	if (UGYInputComponent* InputComponent = Cast<UGYInputComponent>(PlayerInputComponent))
	{

		InputComponent->BindNativeAction(PawnData->InputConfig, GYGameplayTags::InputTag_Move, ETriggerEvent::Triggered,
										 this, &ThisClass::Input_Move, true);

		// UI 토글 PC에 위임
		InputComponent->BindNativeAction(PawnData->InputConfig,GYGameplayTags::InputTag_UI_ToggleSettings,
			ETriggerEvent::Started, this, &UGYHeroComponent::Input_ToggleSettings, false);

		// 어빌리티 입력(Interact 등)은 InputTag → ASC 라우팅으로 일괄 처리
		InputComponent->BindAbilityActions(PawnData->InputConfig, this,
			&ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased);
	}
	else
	{
		GY_ERROR(Player, KHB, "InputComponent가 GYINputComponent가 아님");
	}

}

bool UGYHeroComponent::IsInputBlocked() const
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return false;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return false;

	UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent();
	if (!ASC) return false;

	UGYPawnExtensionComponent* ExtComp = Pawn->FindComponentByClass<UGYPawnExtensionComponent>();
	const UGYPawnData* PawnData = ExtComp ? ExtComp->GetPawnData() : nullptr;
	if (!PawnData || !PawnData->ActionConfig) return false;

	return ASC->HasAnyMatchingGameplayTags(PawnData->ActionConfig->InputBlockTags);
}

void UGYHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	if (IsInputBlocked()) return;

	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator CameraRotation(0.0f, PlayerController->PlayerCameraManager->GetCameraRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = CameraRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = CameraRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}

}

void UGYHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (AGYCharacter* Char = GetPawn<AGYCharacter>())
	{
		if (Char->IsDowned())
		{
			if (const UGYReviveConfig* Config = Char->GetReviveConfig())
			{
				if (Config->GiveUpInputTag.IsValid() && InputTag == Config->GiveUpInputTag)
				{
					Char->StartGiveUpTimer();
					return;
				}
			}
		}
	}

	if (IsInputBlocked()) return;

	APawn* Pawn = GetPawn<APawn>();

	SendGameplayEventLocal(InputTag);
	if (Pawn && !Pawn->HasAuthority())
	{
		ServerSendGameplayEvent(InputTag);
	}

	if (InputTag == GYGameplayTags::InputTag_Attack)
	{
		OnAttackPressed();
		return;
	}

	if (InputTag == GYGameplayTags::InputTag_Parry)
	{
		OnParryPressed();
		return;
	}

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
	{
		ASC->HandleAbilityInputPressed(InputTag);
	}
}

void UGYHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (InputTag == GYGameplayTags::InputTag_Attack)
	{
		OnAttackReleased();
		return;
	}

	if (InputTag == GYGameplayTags::InputTag_Parry)
	{
		OnParryReleased();
		return;
	}

	if (AGYCharacter* Char = GetPawn<AGYCharacter>())
	{
		if (Char->IsDowned())
		{
			if (const UGYReviveConfig* Config = Char->GetReviveConfig())
			{
				if (Config->GiveUpInputTag.IsValid() && InputTag == Config->GiveUpInputTag)
				{
					Char->CancelGiveUpTimer();
					return;
				}
			}
		}
	}

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
	{
		ASC->HandleAbilityInputReleased(InputTag);
	}
}

void UGYHeroComponent::Input_ToggleSettings()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	if (AGYPlayerController* PC = Cast<AGYPlayerController>(Pawn->GetController()))
	{
		PC->RequestToggleSettings();
	}
}

void UGYHeroComponent::OnParryPressed()
{
	if (bParryHeld) return;
	bParryHeld = true;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		ASC->HandleAbilityInputPressed(GYGameplayTags::InputTag_Parry);
}

void UGYHeroComponent::OnParryReleased()
{
	bParryHeld = false;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		ASC->HandleAbilityInputReleased(GYGameplayTags::InputTag_Parry);
}

void UGYHeroComponent::OnAttackPressed()
{
	// 누름 엣지에서만 1회 처리 (Triggered가 매 프레임 발화)
	if (bAttackHeld) return;
	bAttackHeld = true;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent();
	if (!ASC) return;

	// 차지 이미 활성 중이면 새 입력 무시
	bool bComboActive = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive() || !Spec.Ability) continue;
		if (Spec.Ability->GetAssetTags().HasTag(GYGameplayTags::Ability_Attack_Charge)) return;
		if (Spec.Ability->GetAssetTags().HasTag(GYGameplayTags::Ability_Attack_Combo)) bComboActive = true;
	}

	bChargePossible = !bComboActive;

	// 첫 press면 콤보 활성화 시도. 이미 active면 다음 줄의 이벤트로 콤보 체이닝.
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(GYGameplayTags::Ability_Attack_Combo));

	SendGameplayEventLocal(GYGameplayTags::Event_Input_Attack);

	// Block/Parry GA가 활성 중이면 서버에도 이벤트 전달 (콤보 Server_AdvanceCombo와 충돌 방지를 위해 해당 GA만 체크)
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn && !Pawn->HasAuthority())
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive() || !Spec.Ability) continue;
			const FGameplayTagContainer& Tags = Spec.Ability->GetAssetTags();
			if (Tags.HasTag(GYGameplayTags::Ability_Block) || Tags.HasTag(GYGameplayTags::Ability_Parry))
			{
				ServerSendGameplayEvent(GYGameplayTags::Event_Input_Attack);
				break;
			}
		}
	}

	if (HoldToChargeTime > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ChargeThresholdTimer, this, &UGYHeroComponent::OnChargeThreshold, HoldToChargeTime, false);
	}
}

void UGYHeroComponent::OnAttackReleased()
{
	bAttackHeld = false;
	bChargePossible = false;

	GetWorld()->GetTimerManager().ClearTimer(ChargeThresholdTimer);

	SendGameplayEventLocal(GYGameplayTags::Event_Input_AttackRelease);
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn && !Pawn->HasAuthority())
	{
		ServerSendGameplayEvent(GYGameplayTags::Event_Input_AttackRelease);
	}
}

bool UGYHeroComponent::HasChargeDataForCurrentWeapon() const
{
	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	UGYAbilitySystemComponent* ASC = PS ? PS->GetGYAbilitySystemComponent() : nullptr;
	if (!ASC) return false;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UGYPlayerGameplayAbility* PA = Cast<UGYPlayerGameplayAbility>(Spec.Ability);
		if (!PA || !PA->GetAssetTags().HasAny(ChargeAbilityTags)) continue;

		for (const UAbilityFragment* Frag : PA->Fragments)
		{
			const UGYChargeFragment* CF = Cast<UGYChargeFragment>(Frag);
			if (!CF) continue;

			if (CF->GetBestMatchingData(OwnedTags)) return true;
		}
	}
	return false;
}

void UGYHeroComponent::OnChargeThreshold()
{
	if (!bChargePossible || !HasChargeDataForCurrentWeapon()) return;

	GY_LOG(Player, KHB, "OnChargeThreshold fired. ChargeThresholdEventTags: %s", *ChargeThresholdEventTags.ToString());

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UGYPlayerGameplayAbility* PA = Cast<UGYPlayerGameplayAbility>(Spec.Ability);
		if (!PA || !PA->GetAssetTags().HasAny(ChargeAbilityTags)) continue;
		for (const UAbilityFragment* Frag : PA->Fragments)
		{
			const UGYChargeFragment* CF = Cast<UGYChargeFragment>(Frag);
			if (!CF) continue;
			const FGYChargeData* Data = CF->GetBestMatchingData(OwnedTags);
			if (!Data) continue;
			if (Data->ChargeCost.Attribute.IsValid() && ASC->GetNumericAttributeBase(Data->ChargeCost.Attribute) <= 0.f)
				return;
		}
	}

	APawn* Pawn = GetPawn<APawn>();
	for (const FGameplayTag& Tag : ChargeThresholdEventTags)
	{
		SendGameplayEventLocal(Tag);
		if (Pawn && !Pawn->HasAuthority())
		{
			ServerSendGameplayEvent(Tag);
		}
	}

	for (const FGameplayTag& Tag : ChargeThresholdEventTags)
	{
		ASC->HandleAbilityInputPressed(Tag);
	}
}

void UGYHeroComponent::SendGameplayEventLocal(FGameplayTag EventTag)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Pawn;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, EventTag, Payload);
}

void UGYHeroComponent::ServerSendGameplayEvent_Implementation(FGameplayTag EventTag)
{
	SendGameplayEventLocal(EventTag);
}

void UGYHeroComponent::OnRegister()
{
	RegisterInitStateFeature();
	Super::OnRegister();
}



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


