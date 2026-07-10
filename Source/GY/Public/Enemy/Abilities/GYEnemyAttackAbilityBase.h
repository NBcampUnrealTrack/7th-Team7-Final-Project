#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "Enemy/DataTables/EnemyAbilityWeightRow.h"
#include "GYEnemyAttackAbilityBase.generated.h"

class UScoreModifier;
class UEnvQuery;

UENUM(BlueprintType)
enum class EGYEnemyAttackType : uint8
{
	None,
	Melee,
	Ranged,
};

UCLASS()
class GY_API UGYEnemyAttackAbilityBase : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGYEnemyAttackAbilityBase();

	bool CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const;

	float GetRemainingCooldown(const UAbilitySystemComponent* ASC) const;

	float GetTotalDamageScore() const;

	static float CalcAbilityScore(UGYEnemyAttackAbilityBase* Ability, const UAbilitySystemComponent* ASC,
		 AActor* Owner, AActor* Target, const UObject* LastUsed);

	virtual bool CanAttackDistance(AActor* Owner, AActor* Target);
	virtual bool CanAttackAngle(AActor* Owner, AActor* Target);

	// 몽타주의 트레이스 노티파이(EnemyAttackState/LaunchProjectile) 개수 = weight 슬롯 개수.
	// GYEditor의 테이블 동기화와 런타임 콤보 weight 분배가 같은 규칙을 공유한다.
	static int32 CountTraceNotifies(const UAnimMontage* Montage);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void RecalculateAttackDataFromMontage();
#endif
protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	UFUNCTION()
	void PlayAttackMontage();
	UFUNCTION()
	void OnMontageFinished();
	UFUNCTION()
	void OnMontageInterrupted();

	void StartWeaponHitListener();

	UFUNCTION()
	void OnWeaponHit(FGameplayEventData Payload);

	// 트레이스 윈도우(스윙) 시작 시 weight 인덱스 진행 — 빗나간 스윙도 인덱스를 소비한다
	UFUNCTION()
	void OnWeaponWindowBegin(FGameplayEventData Payload);

	virtual const FHitDamageWeight* GetCurrentHitWeight() const;

	// EnemyAbilityWeightTable에서 자기 클래스 행을 찾아 weight를 로드 (부여 시 1회)
	void LoadHitWeightsFromTable();

	// 테이블 행(flat 배열)을 어빌리티에 반영. 콤보는 override하여 스텝별로 분배.
	virtual void ApplyWeightRow(const struct FEnemyAbilityWeightRow& Row);

	// 현재 몇 번째 트레이스 윈도우인지 (INDEX_NONE = 아직 스윙 전, begin 이벤트마다 +1)
	int32 WeaponWindowIndex = INDEX_NONE;
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection",
		meta = (ClampMin = "0", ToolTip = "이 거리보다 가까우면 어빌리티 후보에서 제외. 0이면 비활성화"))
	float MinDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackAngle = 360.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	EGYEnemyAttackType AttackType = EGYEnemyAttackType::None;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Selection")
	FName CalcSocket = TEXT("weapon_tip");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float BaseDamageScore = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	bool bHasCooldown = false;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	float CooldownDuration = 0.f;

	UPROPERTY(Transient)
	FGameplayTagContainer TempCooldownTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Montage")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Montage")
	float PlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
	TArray<FHitDamageWeight> HitDamageWeights;

	UPROPERTY(EditDefaultsOnly, Instanced, Category="Combat|Score")
	TArray<TObjectPtr<UScoreModifier>> ScoreModifiers;

	UPROPERTY(BlueprintReadOnly, Category="Cost")
	float ActivateCost = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag; // 피격 시 카메라 이펙트

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (Categories = "GameplayCue"))
	FGameplayTag AttackCueTag; // 사운드 비주얼 FX 용

	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> EQSAsset;

};
