#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_Interact.generated.h"

// 입력(InputTag.Interact)으로 활성화. ServerOnly — UInteractionComponent의 CurrentInteractable을
// 읽어 최우선 옵션 실행. AbilitySet에 InputTag = Input.Interact 로 부여.
UCLASS()
class GY_API UGA_Interact : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
