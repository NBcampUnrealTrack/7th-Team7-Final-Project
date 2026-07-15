#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Interaction/Interactable.h"
#include "GYCharacter.generated.h"

class UClimbingComponent;
class UHitReactionComponent;
class UPhysicalAnimationComponent;
class UMotionWarpingComponent;
class ULockOnComponent;
class UGYPawnExtensionComponent;
class UAbilitySystemComponent;
class UActiveEquipmentComponent;
class UInteractionComponent;
class UInteractionHighlightComponent;
class UAIPerceptionStimuliSourceComponent;
class UGYOnHitModifierComponent;
class URevivePoolComponent;
class UReviveProgressComponent;
class UGYReviveConfig;
class UGYPlayerActionConfig;

UCLASS()
class GY_API AGYCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface, public IInteractable
{
	GENERATED_BODY()

public:
	AGYCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	UActiveEquipmentComponent* GetActiveEquipmentComponent() const { return ActiveEquipmentComponent; }

	UGYOnHitModifierComponent* GetOnHitModifierComponent() const { return OnHitModifierComponent; }

	UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	UInteractionHighlightComponent* GetInteractionHighlightComponent() const { return InteractionHighlightComponent; }

	ULockOnComponent* GetLockOnComponent() const { return LockOnComponent; }
	UClimbingComponent* GetClimbingComponent() const { return ClimbingComponent; }



	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable)
	void Server_StartFacingLerp(float StartYaw, float TargetYaw, float LerpTime);

	// 보스 사망 연출(시퀀서)이 로컬에서 끝났을 때, 서버에 엔딩 액터 실제 이동을 요청
	UFUNCTION(Server, Reliable)
	void Server_NotifyBossRevealFinished();

	UFUNCTION(BlueprintCallable, Category = "AI|Noise")
	void MakeFootstepNoise();

	UFUNCTION(BlueprintCallable, Category = "AI|Noise")
	void MakeSkillNoise(float Loudness = 1.0f, float MaxRange = 2000.f);

	UFUNCTION(BlueprintPure)
	bool IsDead() const { return bIsDead; }

	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	void Revive(const UGYReviveConfig* Config);

	UFUNCTION(BlueprintCallable)
	void StartGiveUpTimer();

	UFUNCTION(BlueprintCallable)
	void CancelGiveUpTimer();

	UReviveProgressComponent* GetReviveProgressComponent() const { return ReviveProgressComponent; }

	UFUNCTION(BlueprintPure)
	bool IsDowned() const;

	const UGYReviveConfig* GetReviveConfig() const;

protected:
	void HandleDeath();
	void OnHealthChanged(const struct FOnAttributeChangeData& Data);
	void SubscribeHealthDelegate();
	void EnterDownedState(const UGYReviveConfig* Config);
	void GiveUp();

	UFUNCTION()
	void OnRep_bIsDead();

	void EnableRagdoll();
	void DisableRagdoll();
	void ReactivateGameplayAbilities();

	UPROPERTY(ReplicatedUsing=OnRep_bIsDead)
	bool bIsDead = false;
	bool bHealthDelegateBound = false;

protected:
	// 컴포넌트 매니저 통신을 위한 생명주기 함수 오버라이드
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UActiveEquipmentComponent> ActiveEquipmentComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGYOnHitModifierComponent> OnHitModifierComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInteractionHighlightComponent> InteractionHighlightComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULockOnComponent> LockOnComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UClimbingComponent> ClimbingComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat|HitReaction")
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimationComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat|HitReaction")
	TObjectPtr<UHitReactionComponent> HitReactionComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

	/** 캐릭터 초기화 완료 방송 */
	void BroadcastCharacterReady();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HB|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGYPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URevivePoolComponent> RevivePoolComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReviveProgressComponent> ReviveProgressComponent;

	UFUNCTION(Server, Reliable)
	void Server_StartGiveUp();

	UFUNCTION(Server, Reliable)
	void Server_CancelGiveUp();

	void CheckAndForceGiveUpIfAllDown();

	FTimerHandle GiveUpTimerHandle;

	FGenericTeamId TeamId;

	const UGYPlayerActionConfig* GetActionConfig() const;

	void TickFacingLerp();

	FTimerHandle FacingLerpTimer;
	float FacingLerpStartYaw = 0.f;
	float FacingLerpTargetYaw = 0.f;
	float FacingLerpDuration = 0.f;
	float FacingLerpStartTime = 0.f;
};
