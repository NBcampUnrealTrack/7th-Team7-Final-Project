#pragma once

#include "CoreMinimal.h"
#include "GYEnemyCharacterBase.h"
#include "GYChapterBossCharacter.generated.h"

class AGYWeaponActor;
class ALevelSequenceActor;
class ULevelSequencePlayer;
class UBlendSpace;

USTRUCT(BlueprintType)
struct FEnemyWeaponSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AGYWeaponActor> WeaponClass;

	UPROPERTY(EditAnywhere)
	FName AttachSocket = TEXT("Weapon_R");

	UPROPERTY(EditAnywhere)
	FTransform RelativeTransform;

	/** 2페이즈 무기처럼 시작 시 숨겨둘 무기 */
	UPROPERTY(EditAnywhere)
	bool bInitiallyHidden = false;
};

USTRUCT()
struct FBossPhaseWeaponSwap
{
	GENERATED_BODY()

	/** 2페이즈(장검) 로코모션 BlendSpace */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(EditAnywhere)
	FGameplayTag HideWeaponSlot;

	UPROPERTY(EditAnywhere)
	FGameplayTag ShowWeaponSlot;

	/** 장검용 SocketSweep 트레이스 소켓 (비우면 유지) */
	UPROPERTY(EditAnywhere)
	TArray<FName> NewWeaponTraceSockets;
};

UCLASS()
class GY_API AGYChapterBossCharacter : public AGYEnemyCharacterBase
{
	GENERATED_BODY()

public:
	AGYChapterBossCharacter(const FObjectInitializer& ObjectInitializer);

	virtual AGYWeaponActor* GetWeaponBySlot(FGameplayTag SlotTag) const override;

	/** 모든 클라에서 레벨 시퀀스 재생 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCinematic(const FSoftObjectPath& SequencePath);

	/** 서버 대검 -> 장검 전환. 복제되어 클라에도 적용 */
	void SwapToSecondPhaseWeapon();

	bool HasSwappedWeapon() const { return bPhase2Weapon; }

	/** 리스폰 시 페이즈/무기 상태를 1페이즈 기준으로 리셋 */
	virtual void Activate() override;

	virtual void Die() override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SpawnWeapons();

	UFUNCTION()
	void OnRep_Phase2Weapon();

	/** 서버·클라 공통 적용: BlendSpace/몽타주맵/무기 토글/트레이스 소켓 */
	void ApplySecondPhaseWeapon();

	/** 리스폰/복제로 1페이즈 복귀 시: 기본 BlendSpace 복원 + 무기 가시성 + 트레이스 소켓 원복 */
	void ApplyFirstPhaseWeapon();

	/** 시퀀스 재생 중 실보스+부착 무기 숨김/복원. 복원 시 페이즈 상태 기준으로 무기 가시성 재적용 */
	void SetCinematicHidden(bool bNewHidden);

	/** WeaponsToSpawn 초기 정의 + bPhase2Weapon 기준으로 무기 가시성 재적용.
	 *  시퀀스 종료 시 Sequencer Restore State가 되돌린 상태를 교정한다. 서버·클라 공통 실행 가능 */
	void RefreshWeaponVisibility();

	void OnPhaseHealthChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleCinematicFinished();

public:
	UPROPERTY(EditAnywhere, Category = "Combat|Weapon")
	TArray<FEnemyWeaponSpawn> WeaponsToSpawn;

	/** 이 체력 비율 이하로 떨어지면 1회 페이즈 트리거 (BB키 세팅) */
	UPROPERTY(EditAnywhere, Category = "Boss|Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseHealthRatio = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	FBossPhaseWeaponSwap SecondPhaseWeapon;

protected:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<AGYWeaponActor>> EquippedWeapons;

	UPROPERTY(ReplicatedUsing = OnRep_Phase2Weapon)
	bool bPhase2Weapon = false;

	bool bPhaseTriggered = false;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;
};
