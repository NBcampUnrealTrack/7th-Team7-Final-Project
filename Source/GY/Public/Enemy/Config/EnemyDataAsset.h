#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EnemyDataAsset.generated.h"

class UGameplayEffect;
class UGameplayAbility;
class UBehaviorTree;
class UBlackboardData;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None		UMETA(DisplayName = "None"),
	FengMao		UMETA(DisplayName = "FengMao"),
	Sparrow		UMETA(DisplayName = "Sparrow"),
	Melee		UMETA(DisplayName = "Melee"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Boss		UMETA(DisplayName = "Boss"),
};

USTRUCT(BlueprintType)
struct FEnemyVisualConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<TSoftObjectPtr<UMaterialInterface>> Materials;
};

USTRUCT(BlueprintType)
struct FEnemyAnimationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TSoftObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|State")
	TSoftObjectPtr<UAnimSequence> StunSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|State")
	TSoftObjectPtr<UAnimSequence> DeadSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Montage",
		meta = (Categories = "Anim"))
	TMap<FGameplayTag, TSoftObjectPtr<UAnimMontage>> TaggedMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftClassPtr<UAnimInstance> AnimInstanceClass;
};

USTRUCT(BlueprintType)
struct FEnemyAIConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TSoftObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float DetectRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float AttackRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Patrol")
	bool bHasPatrol = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Patrol", meta = (EditCondition = "bHasPatrol"))
	TArray<FVector> PatrolOffsets;

};

USTRUCT(BlueprintType)
struct FEnemyGASConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;

	/** 지속 적용 Effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSoftClassPtr<UGameplayEffect>> PassiveEffects;

	/** AttributeSet 초기화용 Effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TSoftClassPtr<UGameplayEffect> InitStatEffect;

	/** 공격 데미지 Effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Combat")
	TSoftClassPtr<UGameplayEffect> AttackEffect;
};

//TODO 은서: Sound, VFX 넣을지 고려
//TODO 은서: Reward도 고려 필요

USTRUCT(BlueprintType)
struct FEnemyRewardConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	int32 ExpReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	int32 GoldReward = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TSoftObjectPtr<UDataTable> DropTable;
};

UCLASS()
class GY_API UEnemyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName EnemyID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FText EnemyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FEnemyVisualConfig VisualConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	FEnemyAnimationConfig AnimationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	FEnemyAIConfig AIConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	FEnemyGASConfig GASConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FEnemyRewardConfig RewardConfig;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("EnemyConfig", EnemyID);
	}
};
