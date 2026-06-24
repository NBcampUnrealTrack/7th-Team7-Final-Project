// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "GYParkourFragment.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGYParkourFragment : public UAbilityFragment
{
	GENERATED_BODY()
public:

	UGYParkourFragment() { FragmentTag = GYGameplayTags::Ability_Fragment_Parkour; };

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage_Run_Lfoot;
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage_Run_Rfoot;
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage_Stand_Lfoot;
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage_Stand_Rfoot;
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage_Walk_Lfoot;
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage_Walk_Rfoot;

	// 트레이스 파라미터
	UPROPERTY(EditDefaultsOnly)
	float ForwardTraceDistance = 75.f;   // 전방 벽 감지 거리
	UPROPERTY(EditDefaultsOnly)
	float TraceHeightOffset    = 500.f;  // 위에서 아래로 트레이스 시작 높이 - 충분히 높게 잡을 것
	UPROPERTY(EditDefaultsOnly)
	float LowMantleMaxHeight   = 80.f;   // run 몽타주 상한 높이 (cm)
	UPROPERTY(EditDefaultsOnly)
	float MidMantleMaxHeight   = 150.f;  // Walk 몽타주 상한 높이 (cm)
	// MidMantleMaxHeight 초과 → Stand 몽타주
	// 달리기 애니메이션 전용 Z 오프셋
	UPROPERTY(EditDefaultsOnly)
	float RunMantleZOffset = 80.f;
};
