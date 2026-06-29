#include "Camera/GYCameraComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/GYCameraEffectBase.h"
#include "Camera/GYCameraModeData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"
#include "GameFramework/SpringArmComponent.h"
#include "Logging/GYLogManager.h"

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
	//상태를 출력하기위한 로오그
	GY_LOG(Player, KHB, "CameraComp: [%s] -> [%s]", *CurrentState.ToString(), *DesiredState.ToString());

	if (DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (!Pawn || !Pawn->GetRootComponent()) return;

		if (!Pawn->IsLocallyControlled())
		{
			return;
		}

		// 1. 런타임에 스프링 암 동적 생성(NewObject) 및 세팅
		SpringArmComponent = NewObject<USpringArmComponent>(Pawn, TEXT("SpringArmComponent"));
		if (SpringArmComponent)
		{
			SpringArmComponent->SetupAttachment(Pawn->GetRootComponent());
			SpringArmComponent->TargetArmLength = 800.0f;
			SpringArmComponent->bUsePawnControlRotation = false; // 회전 연동 끄기

			SpringArmComponent->SetUsingAbsoluteRotation(true);
			SpringArmComponent->SetUsingAbsoluteLocation(true);
			// 로테이션 DA로 빼기
			SpringArmComponent->bDoCollisionTest = false;

			// 캐릭터(Pawn)가 회전할 때 카메라가 따라서 빙글빙글 돌지 않도록 고정합니다.
			SpringArmComponent->bInheritPitch = false;
			SpringArmComponent->bInheritYaw = false;
			SpringArmComponent->bInheritRoll = false;

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
		// 카메라 모드 초기 세팅
		InitializeCameraModes();
		RegisterCameraTagEvents();
		ResolveCameraMode();
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
	SetIsReplicated(false); //로컬에서 계산
}

void UGYCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

/* 카메라 모드 제어 */
void UGYCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float RealDelta = (GetOwner()->CustomTimeDilation > 0.f)
		                        ? GetWorld()->GetDeltaSeconds() / GetOwner()->CustomTimeDilation
		                        : GetWorld()->GetDeltaSeconds();

	APawn* Pawn = GetPawn<APawn>();

	//로컬에서만 실행
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	if (!CurrentCameraMode)
	{
		return;
	}

	FGYCameraView NewView;

	CurrentCameraMode->UpdateCamera(
		RealDelta,
		NewView);

	// 현재 > 목표 까지 보간
	CurrentView.PivotLocation =
		FMath::VInterpTo(
			CurrentView.PivotLocation,
			NewView.PivotLocation,
			RealDelta,
			NewView.LocationInterpSpeed);

	CurrentView.TargetArmLength =
		FMath::FInterpTo(
			CurrentView.TargetArmLength,
			NewView.TargetArmLength,
			RealDelta,
			NewView.ZoomInterpSpeed);

	CurrentView.FOV =
		FMath::FInterpTo(
			CurrentView.FOV,
			NewView.FOV,
			RealDelta,
			NewView.FOVInterpSpeed);

	CurrentView.SocketOffset =
		FMath::VInterpTo(
			CurrentView.SocketOffset,
			NewView.SocketOffset,
			RealDelta,
			10.f);

	CurrentView.TargetArmRotation =
		FMath::RInterpTo(
			CurrentView.TargetArmRotation,
			NewView.TargetArmRotation,
			RealDelta,
			NewView.RotationInterpSpeed);

	// 이펙트 적용 - duration 누적은 월드 시간 기준 (HitStop의 RealDelta 왜곡 방지)
	for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
	{
		UGYCameraEffectBase* Effect = ActiveEffects[i];

		if (!Effect)
		{
			ActiveEffects.RemoveAt(i);
			continue;
		}

		Effect->UpdateEffect(
			DeltaTime,
			CurrentView);

		if (Effect->IsFinished())
		{
			ActiveEffects.RemoveAt(i);
		}
	}
	ApplyCameraView(CurrentView);
}

void UGYCameraComponent::InitializeCameraModes()
{
	//데이터 에셋 캐싱
	CameraModeMap.Empty();

	for (UGYCameraModeData* Data : CameraModeAssets)
	{
		if (!Data)
		{
			continue;
		}
		CameraModeMap.Add(Data->CameraModeTag, Data);
	}
}

void UGYCameraComponent::RegisterCameraTagEvents()
{
	// 게임플레이 태그 변경 시 카메라 모드 갱신
	UGYAbilitySystemComponent* ASC =
		GetAbilitySystemComponent();

	if (!ASC)
	{
		return;
	}

	for (const auto& Pair : CameraModeMap)
	{
		ASC->RegisterGameplayTagEvent(
			   Pair.Key,
			   EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(
			   this,
			   &UGYCameraComponent::OnCameraTagChanged
		   );
	}
}

void UGYCameraComponent::ResolveCameraMode()
{
	// 우선 순위 비교 후 모드 결정
	UGYAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!ASC)
	{
		SetCameraMode(DefaultCameraMode);
		return;
	}

	UGYCameraModeData* BestMode = DefaultCameraMode;
	int32 BestPriority = -9999;

	for (const auto& Pair : CameraModeMap)
	{
		const FGameplayTag& Tag = Pair.Key;
		UGYCameraModeData* Data = Pair.Value;

		if (!Data)
		{
			continue;
		}

		if (ASC->HasMatchingGameplayTag(Tag))
		{
			if (Data->Priority > BestPriority)
			{
				BestPriority = Data->Priority;
				BestMode = Data;
			}
		}
	}

	if (CurrentCameraMode &&
		CurrentCameraMode->GetCameraData() == BestMode)
	{
		return;
	}

	SetCameraMode(BestMode);
}

void UGYCameraComponent::SetCameraMode(UGYCameraModeData* NewModeData)
{
	if (!NewModeData)
	{
		return;
	}
	// 같으면 생략
	if (CurrentCameraMode && CurrentCameraMode->GetCameraData() == NewModeData)
	{
		return;
	}
	// 이미 존재하는거 제거
	if (CurrentCameraMode)
	{
		CurrentCameraMode->ExitMode();
	}

	CurrentCameraMode = nullptr;

	// 모드 세팅
	if (NewModeData->CameraModeClass)
	{
		CurrentCameraMode =
			NewObject<UGYCameraModeBase>(
				this,
				NewModeData->CameraModeClass);

		if (CurrentCameraMode)
		{
			CurrentCameraMode->Initialize(this, NewModeData);
			CurrentCameraMode->EnterMode();

			// 보간 시작점을 새 모드의 목표값으로 맞춰 첫 프레임에 카메라가 순간 이동하는 현상 방지
			// SetCameraMode가 여러 번 호출될 수 있으므로 최초 1회만 실행
			if (!bInitializedView)
			{
				FGYCameraView InitialView;

				CurrentCameraMode->UpdateCamera(
					0.f,
					InitialView);

				CurrentView = InitialView;

				ApplyCameraView(CurrentView);

				bInitializedView = true;
			}
		}
	}
}

void UGYCameraComponent::ApplyCameraView(const FGYCameraView& View) const
{
	// 뷰 적용
	if (!SpringArmComponent || !CameraComponent)
	{
		return;
	}
	SpringArmComponent->TargetArmLength = View.TargetArmLength;
	SpringArmComponent->SocketOffset = View.SocketOffset;
	SpringArmComponent->SetWorldLocation(View.PivotLocation);
	SpringArmComponent->SetWorldRotation(View.TargetArmRotation);

	CameraComponent->SetFieldOfView(View.FOV);
}

UGYAbilitySystemComponent* UGYCameraComponent::GetAbilitySystemComponent() const
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return nullptr;
	}

	if (IAbilitySystemInterface* ASI =
		Cast<IAbilitySystemInterface>(Pawn))
	{
		return Cast<UGYAbilitySystemComponent>(
			ASI->GetAbilitySystemComponent());
	}

	return nullptr;
}

void UGYCameraComponent::OnCameraTagChanged(
	const FGameplayTag Tag,
	int32 NewCount)
{
	ResolveCameraMode();
}

void UGYCameraComponent::PushCameraEffect(const FGYCameraEffectContext& Context)
{
	const TSubclassOf<UGYCameraEffectBase>* FoundClass =
		CameraEffectMap.Find(Context.Type);

	if (!FoundClass || !(*FoundClass))
	{
		return;
	}

	UGYCameraEffectBase* NewEffect =
		NewObject<UGYCameraEffectBase>(
			this,
			*FoundClass);

	if (!NewEffect)
	{
		return;
	}
	GY_WARN(Player, CYS, "카메라 이펙트 전달: %s",
	        *StaticEnum<EGYCameraEffectType>()->GetNameStringByValue((int64)Context.Type));
	NewEffect->Initialize(Context);

	ActiveEffects.Add(NewEffect);
}
