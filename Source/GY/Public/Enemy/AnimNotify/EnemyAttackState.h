#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "EnemyAttackState.generated.h"

class UGYWeaponHitBox;

UENUM(BlueprintType)
enum class EAttackMode : uint8
{
	SocketSweep,	// 기존 : WeaponTraceSocket 캡슐 스윕
	BodyPart,		// 본체 부위 히트박스 콜리전 토글
	WeaponActor,	// 장착 무기 히트박스 콜리전 토글
};

UCLASS()
class GY_API UEnemyAttackState : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
public:
	UPROPERTY(EditAnywhere, Category = "Trace")
	EAttackMode Mode = EAttackMode::SocketSweep;

	UPROPERTY(EditAnywhere, Category = "Trace", meta = (EditCondition = "Mode != EAttackMode::SocketSweep"))
	FGameplayTag HitBoxTag;
private:
	UGYWeaponHitBox* ResolveHitBox(AActor* Owner) const;

private:
	TArray<FVector> PreCenters;
	TSet<TObjectPtr<AActor>> HitActors;
};
