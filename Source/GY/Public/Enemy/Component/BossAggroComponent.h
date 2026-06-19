#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "BossAggroComponent.generated.h"


class UAIPerceptionComponent;
class AAIController;

USTRUCT(BlueprintType)
struct FAggroEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadOnly)
	float Threat = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float LastUpdateTime = 0.f;
};

/** 가중치/ 튜닝 */
USTRUCT(BlueprintType)
struct FBossAggroWeights
{
	GENERATED_BODY()

	/** 시야에 처음 들어왔을 때 즉시 부여되는 기본 위협도 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float SightOnSpotted = 10.f;

	/** 받은 데미지 1당 누적되는 위협도 배수 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float DamagePerHitMultiplier = 1.f;

	/** 사운드 1회당 위협도 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float NoiseOnHeard = 5.f;

	/** 초당 자연 감쇠량 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float ThreatDecayPerSecond = 0.5f;

	/** 마지막 갱신 후 이 시간이 지나면 ThreatList에서 제거 초 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float ForgetTime = 15.f;

	/** 타겟 전환에 필요한 위협도 차이 — 잦은 타겟 깜빡임 방지 */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	float SwitchHysteresis = 5.f;

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAggroTargetChanged, AActor*, OldTarget, AActor*, NewTarget);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UBossAggroComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossAggroComponent();

	UFUNCTION(BlueprintPure, Category = "Boss|Aggro")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Aggro")
	bool HasTarget() const { return CurrentTarget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Aggro")
	float GetThreatFor(AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Aggro")
	TArray<FAggroEntry> GetTopThreats(int32 Count) const;

	UFUNCTION(BlueprintCallable, Category ="Boss|Aggro")
	void AddThreat(AActor* Actor, float Amount);

	UFUNCTION(BlueprintCallable, Category = "Boss|Aggro")
	void ClearAllThreat();

	UFUNCTION(BlueprintCallable, Category = "Boss|Aggro")
	void ForceTarget(AActor* Actor, float ForcedThreatBonus = 1000.f);

	UPROPERTY(BlueprintAssignable, Category = "Boss|Aggro")
	FOnAggroTargetChanged OnTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BindToPerception();
	void UnbindFromPerception();

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnPerceptionForgotten(AActor* Actor);

	UFUNCTION()
	void HandleTargetChanged(AActor* OldTarget, AActor* NewTarget);

	void TickAggro();
	void InternalAddThreat(AActor* Actor, float Amount);
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Aggro")
	FBossAggroWeights Weights;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Aggro", meta = (ClampMin = "0.05"))
	float UpdateInterval = 0.2f;

	UPROPERTY(VisibleInstanceOnly, Category = "Boss|Aggro|Debug")
	TArray<FAggroEntry> ThreatList;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<AAIController> CachedAIController;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAIPerceptionComponent> CachedPerception;

	FTimerHandle UpdateTimerHandle;
};
