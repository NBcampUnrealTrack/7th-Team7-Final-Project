#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "GYPhantomStrikeAbility.generated.h"

class AGYPhantomActor;

UCLASS()
class GY_API UGYPhantomStrikeAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 시전 몽타주 종료. 팬텀이 아직 살아있으면 소멸까지 대기 후 종료
	virtual void OnMontageFinished() override;

	UFUNCTION()
	void OnSummonPhantom(FGameplayEventData Payload);

	UFUNCTION()
	void OnPhantomDestroyed(AActor* DestroyedActor);

protected:
	/** 스폰할 팬텀 액터 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom")
	TSubclassOf<AGYPhantomActor> PhantomClass;

	/** 타겟(플레이어) 정면 기준 스폰 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom")
	float SpawnOffset = 150.f;

private:
	TWeakObjectPtr<AGYPhantomActor> SpawnedPhantom;
};
