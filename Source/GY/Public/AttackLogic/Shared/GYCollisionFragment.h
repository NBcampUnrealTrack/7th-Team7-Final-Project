#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYCollisionFragment.generated.h"

UENUM(BlueprintType)
enum class EGYCollisionShapeType : uint8
{
	Sphere,
	Box,
	Capsule
};

USTRUCT(BlueprintType)
struct GY_API FGYCollisionShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName BoneName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGYCollisionShapeType ShapeType = EGYCollisionShapeType::Sphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
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
