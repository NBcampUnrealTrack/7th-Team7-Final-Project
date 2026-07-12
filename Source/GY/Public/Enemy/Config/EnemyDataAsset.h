#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EnemyDataAsset.generated.h"

class UEnvQuery;
class UGameplayEffect;
class UGameplayAbility;
class UBehaviorTree;
class UBlackboardData;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None			UMETA(DisplayName = "None"),
	FengMao			UMETA(DisplayName = "FengMao"),
	Sparrow			UMETA(DisplayName = "Sparrow"),
	IggyScorch		UMETA(DisplayName = "IggyScorch"),
	GreyStone		UMETA(DisplayName = "GreyStone"),
	Melee			UMETA(DisplayName = "Melee"),
	Ranged			UMETA(DisplayName = "Ranged"),
	BossFlower		UMETA(DisplayName = "BossFlower"),
	NormalFlower	UMETA(DisplayName = "NormalFlower"),
	PlantRoot		UMETA(DisplayName = "PlantRoot"),
	Knight			UMETA(DisplayName = "Knight"),
	BossBTTest		UMETA(DisplayName = "BossBTTest"),
	Morigesh		UMETA(DisplayName = "Morigesh"),
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|State")
	TSoftObjectPtr<UAnimSequence> StaggerSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|State")
	TSoftObjectPtr<UAnimSequence> ClimbingSequence;

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
	TSoftObjectPtr<UEnvQuery> MovementEQS;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float AbilitySelectionInterval = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float DetectRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Patrol")
	bool bHasPatrol = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Patrol", meta = (EditCondition = "bHasPatrol"))
	TArray<FVector> PatrolOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	FName SightSocketName = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	FRotator SightSocketRotationOffset = FRotator::ZeroRotator;
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

	/** 공격 데미지 Effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Combat")
	TSoftClassPtr<UGameplayEffect> AttackEffect;

	/** 사망 시 실행할 GameplayCue */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Cue",
		meta = (Categories = "GameplayCue"))
	FGameplayTag DeathCueTag;
};

//TODO 은서: Sound, VFX 넣을지 고려
//TODO 은서: Reward도 고려 필요

/**
 * 부위/무기 판정용 히트박스 정의.
 * 공용 BP(EnemyCharacterBase)는 몬스터별 메시를 모르므로,
 * 히트박스는 BP가 아니라 여기(몬스터별 DataAsset)에 정의하고 스폰 시 런타임 생성한다.
 * EnemyAttackState 노티파이(Mode=BodyPart)의 HitBoxTag와 SlotOrPartTag가 일치해야 판정이 켜진다.
 */
USTRUCT(BlueprintType)
struct FEnemyHitBoxDef
{
	GENERATED_BODY()

	/** 노티파이 HitBoxTag와 매칭되는 식별 태그 (Weapon.* 하위) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Weapon"))
	FGameplayTag SlotOrPartTag;

	/** 히트박스를 붙일 스켈레톤 소켓/본 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AttachSocket;

	/** 패링/가드 리액션 계산에 전달되는 본 이름 (보통 AttachSocket과 동일 본) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName HitBoneName;

	/** 소켓 기준 상대 트랜스폼 (소켓을 정확히 배치했다면 0 유지 권장) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform RelativeTransform;

	/** 박스 절반 크기 — gy.ShowHitBox 2 로 실제 게임에서 보면서 조정 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector BoxExtent = FVector(32.f, 32.f, 32.f);
};

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

	UPROPERTY(EditDefaultsOnly, Category="Combat|HitReaction")
	FName HitReactStartBone = TEXT("spine_01");

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

	/** 부위 공격용 히트박스 목록 — 필요 없는 몬스터는 비워둔다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|HitBox")
	TArray<FEnemyHitBoxDef> BodyHitBoxes;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("EnemyConfig", EnemyID);
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration")
	float StaggerDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration")
	float StunDuration = 4.f;
};
