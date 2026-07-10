#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GYANS_AttackTrace.generated.h"

class UStaticMeshComponent;

USTRUCT()
struct FGYHitActorList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Actors;

	FVector LastTraceOrigin = FVector::ZeroVector;

	// EGYCollisionShapeType::Mesh용 콜리전 전용 프록시. 렌더링/충돌 응답 없이 스윕 쿼리에만 사용.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MeshProxy = nullptr;
};

UCLASS()
class GY_API UGYANS_AttackTrace : public UAnimNotifyState
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

private:
	UPROPERTY()
	TMap<TObjectPtr<USkeletalMeshComponent>, FGYHitActorList> HitActorsPerMesh;
};
