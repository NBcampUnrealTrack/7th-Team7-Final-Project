#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "GYPhantomActor.generated.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;

UCLASS()
class GY_API AGYPhantomActor : public AActor
{
	GENERATED_BODY()

public:
	AGYPhantomActor();

	// 스폰 직후 어빌리티가 호출. 히트 이벤트를 받을 Enemy 지정
	void Init(AActor* InOwnerBoss);

	// 타격 판정. 팬텀 애니메이션의 PhantomStrike 노티파이가 호출 (서버에서만 동작)
	void ExecuteStrike();

protected:
	virtual void BeginPlay() override;

	// 애니메이션 종료 시점에 즉시 숨김 + 소멸 연출 (레퍼런스 포즈 노출 방지)
	void OnAnimFinished();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** 스폰 시 1회 재생할 팬텀 애니메이션. PhantomStrike 노티파이 포함 필수 */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom")
	TObjectPtr<UAnimSequenceBase> PhantomAnim;

	/** 타격 판정 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom")
	float StrikeRadius = 150.f;

	/** 숨김 처리 후 액터 파괴까지 여유 시간 (숨겨진 상태라 연출에는 영향 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom")
	float DespawnDelay = 0.5f;

	/** 등장 연출 큐 (예: Teleport.Appear) */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom|Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag AppearCueTag;

	/** 소멸 연출 큐 (예: Teleport.Disappear) */
	UPROPERTY(EditDefaultsOnly, Category = "Phantom|Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag DisappearCueTag;

private:
	void ExecuteLocalCue(const FGameplayTag& CueTag) const;

	UPROPERTY()
	TWeakObjectPtr<AActor> OwnerBoss;

	FTimerHandle FinishTimerHandle;
};
