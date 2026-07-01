#include "AttackLogic/Dodge/GYDodgeInputLogic.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "Core/GameplayTags/EventTags.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

using GYAttributeCostHelpers::ApplyCost;

void UGYDodgeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	const FGYDodgeData* DodgeData = nullptr;
	const FGYDodgeMontageSet* MontageSet = nullptr;

	if (const UGYDodgeFragment* DF = Ability->GetFragment<UGYDodgeFragment>())
	{
		DodgeData = DF->GetBestMatchingData(OwnedTags);
	}

	if (const UGYDodgeMontageFragment* MF = Ability->GetFragment<UGYDodgeMontageFragment>())
	{
		MontageSet = MF->GetBestMatchingSet(OwnedTags);
	}

	if (!DodgeData || !MontageSet)
	{
		Ability->RequestEnd(true);
		return;
	}

	const UGYDodgeFragment* DodgeFragment = Ability->GetFragment<UGYDodgeFragment>();
	CachedDodgeAppliedTag = DodgeFragment ? DodgeFragment->DodgeAppliedTag : FGameplayTag();

	ApplyCost(ASC, DodgeData->StaminaCost);

	float MontageDuration = 0.f;
	// ----------입력에 따른 몽타주 재생 로직
	//서버, 클라 판별
	const bool bIsLocallyControlled = Ability->GetActorInfo().IsLocallyControlled();
	const bool bIsAuthority = Ability->GetActorInfo().IsNetAuthority();

	//서버
	if (bIsAuthority)
	{
		FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(
			Ability->GetCurrentAbilitySpecHandle(),
			Ability->GetCurrentActivationInfo().GetActivationPredictionKey());

		Delegate.AddUObject(this, &UGYDodgeInputLogic::OnTargetDataReceived);

		// (참고) 클라이언트가 이미 서버보다 데이터를 빨리 보냈을 경우를 대비한 안전장치 함수입니다.
		ASC->CallReplicatedTargetDataDelegatesIfSet(
			Ability->GetCurrentAbilitySpecHandle(),
			Ability->GetCurrentActivationInfo().GetActivationPredictionKey()
		);
	}

	//클라이언트
	if (bIsLocallyControlled)
	{
		ACharacter* Character = Cast<ACharacter>(Ability->GetAvatarActorFromActorInfo());
		UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr;

		FVector InputDir = CMC ? CMC->GetLastInputVector() : FVector::ZeroVector;
		InputDir.Z = 0.f;

		if (InputDir.IsNearlyZero()) InputDir = Character->GetActorForwardVector();

		InputDir.Normalize();
		CachedDodgeDirection = InputDir;


		//클라 측 캐릭터 회전

		if (!Ability->GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(GYStateTags::State_LockOn))
		{
			RotateInstanceCharacterMesh(CachedDodgeDirection);
		}

		FVector Forward = Character->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();




		//두 벡터 사이의 각도 구하기 Forward Dot InputDir = Cosθ, Forward X InputDir = Sinθ n (정규화됨)
		// Sinθ/Cosθ = tanθ, arctan(Sinθ/cosθ) = θ

		float cost = FVector::DotProduct(Forward, CachedDodgeDirection);
		float sint = FVector::CrossProduct(Forward, CachedDodgeDirection).Z;
		float DodgeAngle = FMath::RadiansToDegrees(FMath::Atan2(sint, cost));

		//-----------------------------------
		//구조체에 계산한 값(DodgeAngle) 넣기
		//--------------------------------------
		FGYTargetData_DodgeAngle* TargetData = new FGYTargetData_DodgeAngle();
		TargetData->DodgeAngle = DodgeAngle;
		TargetData->InputVector = CachedDodgeDirection;
		//핸들에 만든 구조체 삽입
		FGameplayAbilityTargetDataHandle GYTargetDataHandle;;
		GYTargetDataHandle.Add(TargetData);

		//Prediction Window 열기 : '예측'된 액션으로 등록하여 서버에서 보정 하지 않게 만듬
		FScopedPredictionWindow Window(ASC, true);

		// ASC를 통해 서버로 전송 (RPC)
		ASC->CallServerSetReplicatedTargetData(
			Ability->GetCurrentAbilitySpecHandle(),
			Ability->GetCurrentActivationInfo().GetActivationPredictionKey(),
			GYTargetDataHandle,
			FGameplayTag(),
			ASC->ScopedPredictionKey
		);


		UAnimMontage* SelectedMontage = MontageSet->GetMontageByAngle(DodgeAngle);
		if (!SelectedMontage) { return; }

		// 기존 몽타주 전부 중단
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
			{
				AnimInst->StopAllMontages(0.1f); // 0.1f = 블렌드아웃 시간
			}
		}

		//몽타주 재생
		MontageDuration = Ability->PlayMontageForLogic(SelectedMontage, 1.f);
	}

	//로컬에서만 실행
	if (bIsLocallyControlled)
	{
		DodgeEndTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(MontageDuration, 0.1f));
		DodgeEndTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnDodgeEndFinished);
		DodgeEndTask->ReadyForActivation();
	}
}

void UGYDodgeInputLogic::OnIFrameFinished()
{
	IFrameTask = nullptr;
	RemoveDodgeTag();
}

void UGYDodgeInputLogic::OnDodgeEndFinished()
{
	DodgeEndTask = nullptr;
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYDodgeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (IFrameTask)
	{
		IFrameTask->EndTask();
		IFrameTask = nullptr;
	}
	if (DodgeEndTask)
	{
		DodgeEndTask->EndTask();
		DodgeEndTask = nullptr;
	}
	RemoveDodgeTag();
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Anim_TagApplyStart };
}

void UGYDodgeInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag != GYGameplayTags::Event_Anim_TagApplyStart || !CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !CachedDodgeAppliedTag.IsValid()) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const UGYDodgeFragment* DF = CachedAbility->GetFragment<UGYDodgeFragment>();
	const FGYDodgeData* DodgeData = DF ? DF->GetBestMatchingData(OwnedTags) : nullptr;
	if (!DodgeData) return;

	float InvincibilityDuration = DodgeData->InvincibilityDuration;
	if (const UGYCoreStatAttributeSet* CoreStats = ASC->GetSet<UGYCoreStatAttributeSet>())
		InvincibilityDuration += CoreStats->GetEvasionInvincibilityTime();

	if (InvincibilityDuration <= 0.f) return;

	ASC->AddLooseGameplayTag(CachedDodgeAppliedTag);

	IFrameTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(DodgeData->InvincibilityDuration, KINDA_SMALL_NUMBER));
	IFrameTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnIFrameFinished);
	IFrameTask->ReadyForActivation();
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Dodge,
		GYGameplayTags::Ability_Fragment_DodgeMontage
	};
}

void UGYDodgeInputLogic::OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	if (!CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	//수신 버퍼 제거
	ASC->ConsumeClientReplicatedTargetData(
		CachedAbility->GetCurrentAbilitySpecHandle(),
		CachedAbility->GetCurrentActivationInfo().GetActivationPredictionKey());

	//받은 데이터 유효성 체크
	if (Data.Data.Num() > 0 && Data.Data[0].IsValid())
	{
		//데이터 타입 내 구조체로 변환
		const FGYTargetData_DodgeAngle* DodgeData = static_cast<const FGYTargetData_DodgeAngle*>(Data.Data[0].Get());
		if (DodgeData)
		{
			float ReceivedAngle = DodgeData->DodgeAngle;
			FVector InputVector = DodgeData->InputVector;
			// 서버 측 몽타주 재생 로직

			//서버측 캐릭터 회전
			if (!CachedAbility->GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(GYStateTags::State_LockOn))
			{
				RotateInstanceCharacterMesh(InputVector);
			}

			if (const UGYDodgeMontageFragment* MF = CachedAbility->GetFragment<UGYDodgeMontageFragment>())
			{
				FGameplayTagContainer OwnedTags;
				ASC->GetOwnedGameplayTags(OwnedTags);

				if (const FGYDodgeMontageSet* MontageSet = MF->GetBestMatchingSet(OwnedTags))
				{
					UAnimMontage* SelectedMontage = MontageSet->GetMontageByAngle(ReceivedAngle);
					if (SelectedMontage)
					{
						// 서버 측 몽타주 재생 및 길이 반환
						float ServerMontageDuration = CachedAbility->PlayMontageForLogic(SelectedMontage, 1.f);

						// 서버 측에서 몽타주가 끝날 때 어빌리티를 종료하도록 태스크 실행
						DodgeEndTask = UAbilityTask_WaitDelay::WaitDelay(
							CachedAbility.Get(), FMath::Max(ServerMontageDuration, 0.1f));
						DodgeEndTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnDodgeEndFinished);
						DodgeEndTask->ReadyForActivation();
					}
				}
			}
		}
	}
}

void UGYDodgeInputLogic::RemoveDodgeTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeAppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeAppliedTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeAppliedTag);
	}
}

void UGYDodgeInputLogic::RotateInstanceCharacterMesh(const FVector& InputVector)
{
	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr;

	FRotator InputRotator = InputVector.Rotation();

	Character->SetActorRotation(InputRotator);

}
