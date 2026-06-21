#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "BossAggroComponent.generated.h"


class UAIPerceptionComponent;
class AAIController;

/**
 * BossAggroComponent — AIController에 부착해서 위협도(threat) 기반으로 CurrentTarget을 선정/유지.
 *
 * 입력 (AIPerception stimulus)
 *  - Sight    : 첫 발견 시 SightOnSpotted 가산. Tick에서 시야 유지되면 감쇠량만큼 매번 보충.
 *  - Damage   : 받은 데미지 × DamagePerHitMultiplier 만큼 가산. (GYEnemyVitalAttributeSet → ReportDamageToPerception 경로)
 *  - Hearing  : 이미 ThreatList에 있는 액터만 NoiseOnHeard로 강화. 새 엔트리 생성 X → Hearing 단독으론 Target 안 됨.
 *               (소리만 들었을 때의 Investigation 이동은 AIController가 별도로 InvestigateLocation BB 키로 처리)
 *
 * Tick (UpdateInterval, 기본 0.2s, FTimerManager)
 *  1) 현재 시야에 있는 액터에 감쇠 상쇄분 보충 → 시야 유지되는 동안 threat 0 안 떨어짐
 *  2) 전 엔트리 감쇠 (ThreatDecayPerSecond × DeltaTime)
 *  3) 만료(ForgetTime 초과) / threat 0인 엔트리 제거
 *  4) TopThreat 액터를 CurrentTarget으로 선정. 교체엔 SwitchHysteresis 임계값 — 잦은 깜빡임 방지.
 *
 * 외부 API
 *  - GetCurrentTarget/HasTarget/GetThreatFor/GetTopThreats : 조회
 *  - AddThreat        : 임의로 가중 (이벤트성)
 *  - ForceTarget      : 강제 지정 (보너스 threat + CurrentTarget 즉시 세팅)
 *  - ClearAllThreat   : 전체 초기화
 *  - OnTargetChanged  : Target 변경 브로드캐스트 (combat tag on/off 등에 사용)
 *
 * 팩션 필터
 *  - OnPerceptionUpdated 진입 시 Enemy 팩션 가진 액터는 무시 (아군끼리 어그로 안 끌림)
 *  - Tick의 시야 보충에서도 동일 필터 적용
 *
 * 서버 권한
 *  - 모든 변경 로직은 GetOwner()->HasAuthority() 인 경우에만 실행. ThreatList/CurrentTarget은 복제하지 않음.
 */

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
