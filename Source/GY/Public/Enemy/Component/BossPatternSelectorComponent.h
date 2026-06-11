#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "Components/ActorComponent.h"
#include "BossPatternSelectorComponent.generated.h"

USTRUCT(BlueprintType)
struct FBossPatternChain
{
	GENERATED_BODY()

	/** 후속 패턴 식별 태그 */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag NextPatternTag;

	/** 체인 발동 확률 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Probability = 0.5f;

	/** 체인 발동 시 거리 조건 무시 */
	UPROPERTY(EditDefaultsOnly)
	bool bIgnoreDistanceCheck = false;

	/** 체인 발동 시 쿨다운 무시 */
	UPROPERTY(EditDefaultsOnly)
	bool bIgnoreCooldown = false;
};

USTRUCT(BlueprintType)
struct FBossPatternEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 패턴 식별 태그. 체인에서 키로 사용 */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag PatternTag;

	/** 거리 조건 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float MinDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float MaxDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float BaseWeight = 1.f;

	/**
	 * 거리에 따른 가중치 배수 곡선.
	 *   X = 타겟과의 거리 (uu)
	 *   Y = 가중치 배수 (BaseWeight × Y)
	 * 곡선 비워두면 항상 1.0 (BaseWeight만 사용)
	 *
	 * 예시 (Lunge 돌진):
	 *   거리 0~200   : 0.1  (근접에선 거의 안 씀)
	 *   거리 300~600 : 1.0
	 *   거리 600~1500: 3.0  (멀수록 강하게 선택)
	 */
	UPROPERTY(EditDefaultsOnly)
	FRuntimeFloatCurve DistanceWeightCurve;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float Cooldown = 3.f;

	UPROPERTY(EditDefaultsOnly)
	bool bAllowConsecutive = false;

	UPROPERTY(EditDefaultsOnly, Category = "Chain")
	TArray<FBossPatternChain> Chains;

	UPROPERTY(EditDefaultsOnly)
	FName DebugName = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatternChosen, TSubclassOf<UGameplayAbility>, AbilityClass);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UBossPatternSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossPatternSelectorComponent();

	void InitializePatterns(const TArray<FBossPatternEntry>& InPatterns);

	UFUNCTION(BlueprintPure, Category = "Boss|Pattern")
	bool HasReadyPattern(AActor* Target) const;

	TSubclassOf<UGameplayAbility> SelectNextPattern(AActor* Target);
	TSubclassOf<UGameplayAbility> GetPendingAbility() const { return PendingAbility; }
	TSubclassOf<UGameplayAbility> ConsumePendingAbility();

	UFUNCTION(BlueprintCallable, Category = "Boss|Pattern")
	void SetPendingAbility(TSubclassOf<UGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, Category = "Boss|Pattern")
	void NotifyPatternFinished(TSubclassOf<UGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintPure, Category ="Boss|Pattern")
	int32 GetPatternCount() const { return Patterns.Num(); }

	UPROPERTY(BlueprintAssignable, Category ="Boss|Pattern")
	FOnPatternChosen OnPatternChosen;
protected:
	TSubclassOf<UGameplayAbility> TrySelectChain(float Distance);
	TSubclassOf<UGameplayAbility> SelectedByWeightedRandom(float Distance);
	bool IsPatternAvailable(const FBossPatternEntry& Pattern, float Distance, bool bIgnoreCooldown, bool bIgnoreDistance) const;
	float ComputedDynamicWeight(const FBossPatternEntry& Pattern, float Distance) const;
	const FBossPatternEntry* FindPatternByTag(FGameplayTag Tag) const;
	float GetDistanceToTarget(AActor* Target) const;
	void RegisterSelected(const FBossPatternEntry& Selected);
protected:
	UPROPERTY(Transient)
	TArray<FBossPatternEntry> Patterns;

	UPROPERTY(Transient)
	TMap<TSubclassOf<UGameplayAbility>, float> LastUsedTime;

	UPROPERTY(Transient)
	FGameplayTag LastSelectedTag;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> LastSelectedAbility;

	UPROPERTY(Transient)
	bool bPreviousFinished = false;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> PendingAbility;
};
