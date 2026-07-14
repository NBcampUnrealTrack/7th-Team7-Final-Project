#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYBossPhaseAbility.generated.h"

class AGYBossCharacterBase;
class UBossPhaseComponent;
class AAreaWarningActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSubAbilityFinished, TSubclassOf<UGameplayAbility>, AbilityClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhaseSequenceFinished);

/**
 * 보스 페이즈 전환의 컨테이너 어빌리티.
 *
 * - PhaseTrigger(체력 임계) 도달 시 PhaseComponent 큐에 자기 자신이 들어가 활성화된다.
 * - 활성화되면 ApplyPhaseEntry(무적/캔슬/GE/Cue), OnPhaseExecute(BP 정의 연출),
 *   그리고 SubAbilities 가 있으면 ExecuteSubAbilitySequence 로 순차 실행한다.
 * - 모든 SubAbility 가 끝나면 OnSequenceFinished 가 발사되고,
 *   bAutoFinishAfterSequence 가 켜져 있으면 FinishPhase 가 자동 호출되어 페이즈 종료.
 *
 * 한 페이즈에 포함될 모든 컨텐츠(연출 + 시퀀스 + Entry/Exit 효과)를 BP 한 곳에서 관리한다.
 */
UCLASS()
class GY_API UGYBossPhaseAbility : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGYBossPhaseAbility();

	/**
	 * 주어진 PhaseAbility 클래스와 그 SubAbilities/AbilitiesToGrantOnExit 를 OutSet 에 모은다.
	 *
	 * BossCharacter::GrantDefaultAbilities 가 보스 데이터의 PhaseTriggers 를 순회하며
	 * 이 헬퍼로 모든 어빌리티를 수집한 뒤 ASC 에 일괄 GiveAbility 한다.
	 * 중복은 TSet 이 알아서 걸러준다.
	 */
	static void CollectAbilities(
		TSubclassOf<UGYBossPhaseAbility> PhaseClass,
		TSet<TSubclassOf<UGameplayAbility>>& OutSet);

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|SubAbility")
	bool ActivateSubAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase|SubAbility")
	FOnSubAbilityFinished OnSubAbilityFinished;

	/** SubAbilities를 순서대로 활성화. 시퀀스 종료 시 OnSequenceFinished 발사 */
	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Sequence")
	void ExecuteSubAbilitySequence();

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase|Sequence")
	FOnPhaseSequenceFinished OnSequenceFinished;

	void CachingParticipants();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase")
	void ApplyPhaseEntry();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase")
	void ApplyPhaseExit();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Phase")
	void OnPhaseExecute();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase")
	void FinishPhase();

	UFUNCTION(BlueprintPure, Category = "Boss|Phase|Participants")
	TArray<APlayerState*> GetCachedParticipants() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Phase|Participants")
	int32 GetCachedParticipantCount() const { return CachedParticipants.Num(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Phase|Participants")
	TArray<APawn*> GetCachedParticipantPawns() const;

	UFUNCTION(BlueprintCallable, Category ="Boss|Phase|Participants")
	void ApplyGEToAllParticipants(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Participants")
	void ExecuteCueOnAllParticipants(FGameplayTag CueTag);

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Participants")
	void ApplyGEToParticipant(APlayerState* Target, TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Self")
	void GrantAbilitiesNow();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Self")
	void RemoveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& Abilities);
private:
	UFUNCTION()
	void HandleSubAbilityEnded(UGameplayAbility* Ability);

	void AdvanceSequence();
	void ActivateCurrentSequenceAbility();

	UFUNCTION()
	void OnBossMinionCountChanged(int32 NewCount);

	void EnterMinionGateStun();
	void OnMinionGateTimeout();

	void OnStunTagChanged(FGameplayTag CallbackTag, int32 NewCount);

	void RemoveInvulnerabilityTagsNow();
	void UnbindStunTagObserver();

	AGYBossCharacterBase* GetBoss();
	UBossPhaseComponent*  GetPhaseComp();

	void EndAOEWindow();

	void AddCameraTagsToParticipants();
	void RemoveCameraTagsFromParticipants();

	FTimerHandle MinionGateTimeoutTimer;
	FTimerHandle PunishResolveTimer;
	FActiveGameplayEffectHandle StunEffectHandle;
	FDelegateHandle StunTagDelegateHandle;
	bool bMinionGateTriggered = false;

	TWeakObjectPtr<AGYBossCharacterBase> CachedBoss;
	TWeakObjectPtr<UBossPhaseComponent>  CachedPhaseComp;

	/** 게이트 시작 시 스폰한 경고 액터 (성공 시 Cancel, 실패 시 그 위치에 타격) */
	TWeakObjectPtr<AAreaWarningActor> ActiveTimeoutWarning;

protected:
	/** 진입 시 부여할 무적/슈퍼아머 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 보스 ASC 에 loose tag 로 추가되는 태그들. 일반적으로 무적/슈퍼아머 등 짧은 무적 표현용. 페이즈 종료 또는 어빌리티 캔슬 시 자동 제거된다."))
	FGameplayTagContainer InvulnerabilityTags;

	/** 페이즈 진입 시 각 참가자(플레이어) ASC에 부착할 카메라 연출용 태그. 페이즈 종료 시 자동 제거된다. (복제됨) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 각 참가자(플레이어) ASC 에 replicated loose tag 로 부착. 클라이언트 카메라 위치 변경 트리거용. 페이즈 종료/캔슬 시 자동 제거."))
	FGameplayTagContainer ParticipantCameraTags;

	/** 진입 시 캔슬할 진행 중 어빌리티 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 보스가 현재 사용 중인 어빌리티 중 이 태그들을 가진 어빌리티는 즉시 CancelAbilities 로 중단된다."))
	FGameplayTagContainer CancelAbilitiesWithTags;

	/** 진입 시 강제로 클렌즈할 ActiveGameplayEffect 의 매칭 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 보스 ASC 에 붙어 있던 ActiveGameplayEffect 중, 이 컨테이너의 태그를 (Asset/Granted Tag 매칭) 가진 효과를 즉시 강제 제거한다. 무적 부여 직전에 남아 있던 Stagger/Stun 같은 CC 잔재 청소용. 일반적으로 Effect.Type.CC 만 넣어두면 된다."))
	FGameplayTagContainer CleanseTagsOnEntry;

	/** 진입 시 보스에게 적용할 영구 GE (예 : 스탯 강화 ) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 보스 ASC 에 적용할 영구 GameplayEffect 들. 예: 페이즈 스탯 강화. 페이즈 어빌리티가 캔슬되면 자동 제거된다."))
	TArray<TSubclassOf<UGameplayEffect>> EntryGameplayEffects;

	/** 진입 시 모든 참가자에게 적용할 즉시성 GE (예 : 광역 데미지) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 모든 보스 참가자(플레이어)에게 적용할 즉시 GE 들. 예: 페이즈 전환 데미지 / 디버프."))
	TArray<TSubclassOf<UGameplayEffect>> EntryParticipantGameplayEffects;

	/** 진입 시 보스 ASC에서 발사할 Cue */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 보스 본인의 ASC 에서 ExecuteGameplayCue 로 한 번 발사할 Cue 태그. 비주얼/사운드 이펙트."))
	FGameplayTag EntryCueTag;

	/** 진입 시 각 참가자 ASC에서 발사할 Cue (화면 흔들림 등등)*/
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry",
		meta = (ToolTip = "페이즈 진입 시 각 참가자(플레이어) ASC 에서 발사할 Cue. 카메라 셰이크, 풀스크린 효과 등 클라이언트 측 연출용."))
	FGameplayTag EntryParticipantCueTag;

	/** 종료 시 Grant할 새 GA (페이즈별 젼용 패턴) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Exit",
		meta = (ToolTip = "페이즈 종료 시 보스 ASC 에 새로 부여될 어빌리티. 다음 페이즈에서만 사용 가능한 패턴을 잠금 해제할 때 사용."))
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrantOnExit;

	/** 종료 시 회수 할 GA */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Exit",
		meta = (ToolTip = "페이즈 종료 시 보스 ASC 에서 제거할 어빌리티. 이전 페이즈 전용 패턴을 더 이상 못 쓰게 막을 때 사용."))
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToRemoveOnExit;

	/** 종료 시 보스 ASC에서 발사할 Cue */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Exit",
		meta = (ToolTip = "페이즈 종료 시 보스 본인의 ASC 에서 발사할 Cue. 페이즈 전환 종료 연출용."))
	FGameplayTag ExitCueTag;

	/** 페이즈 진입 시 OnPhaseExecute 직후 순차 활성화할 SubAbility 리스트 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Sequence",
		meta = (ToolTip = "페이즈 진입 후 OnPhaseExecute 직후 순서대로 활성화될 어빌리티 시퀀스. 이 리스트의 어빌리티들은 GrantDefaultAbilities 단계에서 ASC 에 자동 부여된다."))
	TArray<TSubclassOf<UGameplayAbility>> SubAbilities;

	/** SubAbility 간 딜레이(초). 한 GA가 끝나고 다음 GA 활성화까지 대기 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Sequence", meta = (ClampMin = "0",
		ToolTip = "이전 SubAbility 가 종료되고 다음 SubAbility 가 활성화되기까지 대기할 시간(초). 0 이면 즉시 다음 어빌리티 활성화."))
	float DelayBetweenAbilities = 0.f;

	/** true면 시퀀스가 끝나는 즉시 FinishPhase 호출 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Sequence",
		meta = (ToolTip = "true 이면 SubAbilities 시퀀스가 모두 끝난 직후 FinishPhase 를 자동 호출하여 페이즈 어빌리티 자체를 종료한다."))
	bool bAutoFinishAfterSequence = false;

	/** true면 SubAbility로 소환한 잡몹이 모두 죽을 때까지 무적 유지 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (ToolTip = "true면 SubAbility로 소환한 잡몹이 모두 죽을 때까지 무적 유지"))
	bool bUseMinionGate = false;

	/** 잡몹 전멸 시 보스에 적용할 Stun GE. Duration은 GE 자체에 설정. State.Hit.Stun 태그 부여 권장. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
	meta = (EditCondition = "bUseMinionGate",
		ToolTip = "잡몹 전멸 시 보스에 적용할 Stun GE. Duration은 GE 자체에 설정. State.Hit.Stun 태그를 부여하는 Duration GE 권장 (예: GE_Stun)."))
	TSubclassOf<UGameplayEffect> MinionGateStunEffect;

	/** 켜면 MinionGateTimeoutDuration 안에 잡몹 못 잡을 때 PunishAbility 발동 후 무적 해제 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
	meta = (EditCondition = "bUseMinionGate",
		ToolTip = "켜면 MinionGateTimeoutDuration 안에 잡몹 못 잡을 때 PunishAbility 발동 후 무적 해제"))
	bool bUseMinionTimeoutPunish = false;

	/** 잡몹 처치 제한 시간(초). 만료 시 PunishAbility 발동 + 무적 해제 + Phase 종료 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish", ClampMin = "0.0",
			ToolTip = "잡몹 처치 제한 시간(초). 만료 시 PunishAbility 발동 + 무적 해제 + Phase 종료"))
	float MinionGateTimeoutDuration = 30.f;

	/** 타임아웃 시 발동할 페널티 어빌리티 (광역 데미지 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish",
			ToolTip = "타임아웃 시 발동할 페널티 어빌리티 (광역 데미지 등)"))
	TSubclassOf<UGameplayAbility> MinionTimeoutPunishAbility;

	/** 게이트 시작 시 아레나 중앙에 미리 스폰되어 타임아웃까지 커지는 경고 액터. 성공 시 취소, 실패 시 그 자리에 타격. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish",
			ToolTip = "MinionGate 시작 시 아레나 중앙에 스폰되어 MinionGateTimeoutDuration 동안 커지는 경고 액터. 잡몹 전멸(성공) 시 폭발 없이 제거, 타임아웃(실패) 시 PunishAbility 타격이 그 위치에 발생."))
	TSubclassOf<AAreaWarningActor> TimeoutWarningClass;

	/** 경고 액터의 최종 반경(cm). PunishAbility 투사체 ImpactRadius 와 같게 맞추는 것을 권장. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish", ClampMin = "0.0",
			ToolTip = "경고 원이 다 자랐을 때의 반경(cm). 벌칙 투사체의 ImpactRadius 와 동일하게 맞추면 시각/타격 크기가 일치한다."))
	float TimeoutWarningRadius = 800.f;

	/** 아레나 중앙 지점으로 사용할 레벨 액터 태그. PunishAbility 의 ArenaCenterTag 와 동일해야 위치가 맞는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish",
			ToolTip = "경고/타격 중심으로 쓸 레벨 액터의 Actor Tag. 해당 태그를 가진 액터가 없으면 보스 위치로 폴백."))
	FName ArenaCenterTag = TEXT("ArenaCenter");

	/** 경고 액터 스폰 Z 오프셋(cm). 스폰 시 바닥으로 스냅되므로 z-fighting 방지용 미세 조정에만 사용. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish",
			ToolTip = "바닥 스냅 지점에 더할 Z 오프셋(cm). 스폰 시 아래로 라인트레이스해서 지면에 붙이므로 +2~5 정도의 미세 조정만 필요."))
	float TimeoutWarningZOffset = 5.f;

	/** 타임아웃 벌칙 발동 후 페이즈 종료까지의 지연(초). 투사체가 착지·데미지 적용할 시간 확보용. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|MinionGate",
		meta = (EditCondition = "bUseMinionTimeoutPunish", ClampMin = "0.0",
			ToolTip = "타임아웃에 PunishAbility 를 켠 뒤 이 시간(초)이 지나서야 FinishPhase 를 호출한다. 즉시 종료하면 방금 켠 벌칙 어빌리티가 ClearAbility 로 취소되어 투사체 히트 이벤트를 못 받아 데미지가 안 들어간다. 투사체 낙하 시간(SpawnHeight/DropSpeed)보다 넉넉히 크게."))
	float PunishResolveDelay = 2.f;
protected:
	/** ActivateAbility 시점의 참가자 스냅샷 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerState>> CachedParticipants;

	/** Entry에서 적용한 영구 GE Handle (Cancel/EndAbility 시 회수용) */
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> ActiveEntryEffectHandles;

	/** Exit에서 Grant할 GA Spec Handle */
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;

	/** 페이즈 동안 Grant한 임시 GA 핸들 */
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> TemporaryGrantedHandles;

	bool bPhaseFinished = false;

	bool bSequenceRunning = false;
	int32 CurrentSubAbilityIndex = INDEX_NONE;
	FTimerHandle SequenceDelayTimer;

};

