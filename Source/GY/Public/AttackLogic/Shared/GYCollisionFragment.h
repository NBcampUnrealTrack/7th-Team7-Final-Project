#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYCollisionFragment.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EGYCollisionShapeType : uint8
{
	Sphere,
	Box,
	Capsule,
	Mesh
};

USTRUCT(BlueprintType)
struct GY_API FGYCollisionShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName BoneName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGYCollisionShapeType ShapeType = EGYCollisionShapeType::Sphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType != EGYCollisionShapeType::Mesh", EditConditionHides))
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType != EGYCollisionShapeType::Mesh", EditConditionHides))
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType == EGYCollisionShapeType::Sphere", EditConditionHides))
	float SphereRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType == EGYCollisionShapeType::Box", EditConditionHides))
	FVector BoxHalfExtent = FVector(25.f, 25.f, 25.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType == EGYCollisionShapeType::Capsule", EditConditionHides))
	float CapsuleRadius = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType == EGYCollisionShapeType::Capsule", EditConditionHides))
	float CapsuleHalfHeight = 50.f;

	// 무기 실제 콜리전(예: 소드 블레이드 컨벡스)을 그대로 사용. BoneName 소켓에 오프셋 없이 부착되므로
	// 위치/회전은 소켓 트랜스폼과 항상 일치한다 (Offset/Rotation 미사용).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (EditCondition = "ShapeType == EGYCollisionShapeType::Mesh", EditConditionHides))
	TObjectPtr<UStaticMesh> CollisionMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bShowDebug = false;
};

USTRUCT(BlueprintType)
struct GY_API FGYCollisionSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGYCollisionShapeData> Shapes;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYCollisionFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYCollisionFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	TMap<FGameplayTag, FGYCollisionSet> CollisionSets;

	const TArray<FGYCollisionShapeData>* GetBestMatchingShapes(const FGameplayTagContainer& OwnedTags) const;
};
