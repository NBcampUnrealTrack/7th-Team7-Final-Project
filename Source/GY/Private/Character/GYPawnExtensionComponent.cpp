// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYPawnExtensionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Character/GYPawnData.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"
#include "GameFramework/Pawn.h"
#include "Logging/GYLogManager.h"
#include "Player/GYPlayerState.h"

// 이 extcomp의 이름은 PawnExtension 임
const FName UGYPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UGYPawnExtensionComponent::UGYPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

const UGYPawnData* UGYPawnExtensionComponent::GetPawnData() const
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const AGYPlayerState* PS = Pawn->GetPlayerState<AGYPlayerState>())
		{
			return PS->GetPawnData();
		}
	}
	return nullptr;
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

	if (CurrentState == GYGameplayTags::InitState_Spawned &&
		DesiredState == GYGameplayTags::InitState_DataAvailable)
	{
		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			if (!GetController<AController>()) { return false; } // 컨트롤러 빙의 대기
		}

		// PawnData·ASC 모두 PlayerState 경유(GameMode가 Experience로 PS에 PawnData set).
		// PS와 PS의 PawnData가 준비돼야 DataAvailable로 진행.
		const AGYPlayerState* PS = Pawn->GetPlayerState<AGYPlayerState>();
		if (!PS || !PS->GetPawnData())
		{
			return false;
		}

		return true;
	}
	if (CurrentState == GYGameplayTags::InitState_DataAvailable &&
		DesiredState == GYGameplayTags::InitState_DataInitialized)
	{
		return Manager->HaveAllFeaturesReachedInitState(Pawn, GYGameplayTags::InitState_DataAvailable);
	}

	if (CurrentState == GYGameplayTags::InitState_DataInitialized &&
		DesiredState == GYGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UGYPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	GY_LOG(Player, KHB, "ExtComp : [%s] -> [%s]", *CurrentState.ToString(), *DesiredState.ToString());

	if (DesiredState == GYGameplayTags::InitState_DataAvailable)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (!Pawn) return;

		AGYPlayerState* PS = Pawn->GetPlayerState<AGYPlayerState>();
		if (!PS) return; // CanChangeInitState에서 PS+PawnData를 요구하므로 여기선 항상 유효

		UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent();
		if (!ASC) return;

		// ① ASC↔폰 바인딩(서버/클라 공통). 같은 아바타로 이미 묶였으면 스킵.
		const bool bAlreadyBound = ASC->AbilityActorInfo.IsValid()
			&& ASC->AbilityActorInfo->AvatarActor.Get() == Pawn;
		if (!bAlreadyBound)
		{
			ASC->InitAbilityActorInfo(PS, Pawn);
		}

		if (!Pawn->HasAuthority()) return;

		const UGYPawnData* PawnData = PS->GetPawnData();

		// ② 진영 태그 부여(서버). 데이터(PawnData)에서 읽음.
		if (PawnData && PawnData->Faction.IsValid())
		{
			ASC->AddLooseGameplayTag(PawnData->Faction, 1, EGameplayTagReplicationState::TagOnly);
		}

		// ③ base 어트리뷰트 값 초기화(서버). 값 정책은 PlayerState 소관.
		PS->InitializeBaseAttributes();

		// ④ AbilitySet 부여(서버). ASC 초기화는 PawnExtension이 전담한다.
		if (PawnData)
		{
			CachedASC = ASC;
			for (const UAbilitySet* AbilitySet : PawnData->AbilitySets)
			{
				if (AbilitySet)
				{
					AbilitySet->GiveToAbilitySystem(ASC, &GrantedHandles);
				}
			}
		}
	}
}

void UGYPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// 다른 컴포넌트(예: HeroComponent)가 준비되었다는 소식을 들으면, 나도 다음 단계로 넘어갈 수 있는지 체크합니다.
	// If another feature is now in DataAvailable, see if we should transition to DataInitialized
	// 본인이 아닐 때 실행
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		if (Params.FeatureState == GYGameplayTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}

void UGYPawnExtensionComponent::CheckDefaultInitialization()
{
	GY_LOG(Player, KHB, "ExtComp: CheckDefaultInitialization 호출됨");

	CheckDefaultInitializationForImplementers();
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
	// 부여한 AbilitySet 회수(서버). 폰 파괴 시 ASC에서 어빌리티/GE 제거.
	if (CachedASC.IsValid())
	{
		GrantedHandles.TakeFromAbilitySystem(CachedASC.Get());
		CachedASC.Reset();
	}

	UnregisterInitStateFeature(); // 등록 해제
	Super::EndPlay(EndPlayReason);
}



