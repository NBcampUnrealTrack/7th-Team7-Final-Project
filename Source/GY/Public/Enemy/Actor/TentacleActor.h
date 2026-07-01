#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "TentacleActor.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class USkeletalMeshComponent;

/**
 * 어빌리티가 스폰하는 촉수 프롭 액터.
 * 자체 ASC 는 없고, GetAbilitySystemComponent 는 Owner(=Boss) 의 ASC 를 반환한다.
 * 이 덕분에 촉수 몽타주의 MeleeTrace 노티파이가 히트 이벤트를 자기 자신(촉수)에게 보내도,
 * 실제로는 보스 ASC 로 라우팅되어 보스가 활성화한 어빌리티에서 처리 가능.
 */
UCLASS()
class GY_API ATentacleActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATentacleActor();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Tentacle")
	USkeletalMeshComponent* GetSkeletalMesh() const { return SkeletalMesh;}

	UFUNCTION(BlueprintCallable, Category = "Tentacle")
	void StartFadeOutAndDestroy(float FadeDuration = 0.3f);

	/**
	 * 서버에서 호출. 복제 변수에 몽타주를 세팅하여 모든 클라이언트가 OnRep 으로 동기 재생하게 한다.
	 * 서버 측 실제 Montage_Play 는 어빌리티의 PlayMontageOnMesh 태스크가 담당한다.
	 */
	void StartMontagePlayback(UAnimMontage* Montage, float Rate = 1.f);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ActiveMontage();

private:
	UFUNCTION()
	void HandleFadeFinished();

public:
	/**
	 * 무기 트레이스에 사용할 소켓 이름 배열. AGYEnemyCharacterBase 의 동명 필드와 같은 규격.
	 * BeginPlay 에서 SkeletalMesh 의 소켓 중 WeaponTraceBonePrefix 로 시작하는 것들을
	 * 자동 수집하여 채운다. EnemyWeaponTrace 노티파이가 이 배열을 읽어서 트레이스한다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tentacle|Trace")
	TArray<FName> WeaponTraceSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tentacle|Trace", meta = (ClampMin = "0.0"))
	float WeaponTraceRadius = 25.f;

	/** 이 접두어로 시작하는 소켓들만 WeaponTraceSockets 에 자동 등록된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tentacle|Trace")
	FString WeaponTraceBonePrefix = TEXT("WeaponTrace_");

protected:
	/** SkeletalMesh 의 모든 소켓 중 접두어 매칭되는 것들을 정렬하여 WeaponTraceSockets 에 채운다. */
	void CachedWeaponTraceSockets();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tentacle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveMontage)
	TObjectPtr<UAnimMontage> ActiveMontage;

	UPROPERTY(Replicated)
	float ActiveMontagePlayRate = 1.f;

private:
	FTimerHandle FadeTimerHandle;
};
