// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYPawnExtensionComponent.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"

// 이 extcomp의 이름은 PawnExtension 임
const FName UGYPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UGYPawnExtensionComponent::UGYPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGYPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
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
		// Pawn data is required.
		if (!PawnData)
		{
			return false;
		}
		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			if (!GetController<AController>()) { return false; } // 컨트롤러 빙의 대기
		}

		return true;
	}
	else if (CurrentState == GYGameplayTags::InitState_DataAvailable && DesiredState ==
		GYGameplayTags::InitState_DataInitialized)
	{
		return Manager->HaveAllFeaturesReachedInitState(Pawn, GYGameplayTags::InitState_DataAvailable);
	}
	else if (CurrentState == GYGameplayTags::InitState_DataInitialized && DesiredState == GYGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UGYPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	//상태가 변했을 때 필요한 세팅(예: DataAvailable이 되면 PawnData를 캐싱함)
	//이건 각 컴포넌트가 알아서 구현함.
}

void UGYPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// 다른 컴포넌트(예: HeroComponent)가 준비되었다는 소식을 들으면, 나도 다음 단계로 넘어갈 수 있는지 체크합니다.
	CheckDefaultInitialization();
}

void UGYPawnExtensionComponent::CheckDefaultInitialization()
{
	// 조건들을 검사하고 ContinueInitStateChain()을 호출하여 상태 머신을 굴려주는 함수입니다.
	static const TArray<FGameplayTag> StateChain = {
		GYGameplayTags::InitState_Spawned, GYGameplayTags::InitState_DataAvailable,
		GYGameplayTags::InitState_DataInitialized, GYGameplayTags::InitState_GameplayReady
	};

	ContinueInitStateChain(StateChain);
}



void UGYPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();
	RegisterInitStateFeature(); // 관리 매니저에 등록
}
void UGYPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	ensure(TryToChangeInitState(GYGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UGYPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature(); // 등록 해제
	Super::EndPlay(EndPlayReason);
}



