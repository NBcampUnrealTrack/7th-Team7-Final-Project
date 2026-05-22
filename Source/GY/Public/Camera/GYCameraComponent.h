// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GYCameraComponent.generated.h"


enum class EGYCameraEffectType : uint8;
struct FGYCameraEffectContext;
class UGYCameraEffectData;
class UGYCameraEffectBase;
class UGYAbilitySystemComponent;
class UGYCameraModeData;
class UCameraComponent;
class USpringArmComponent;

USTRUCT()
struct FGYCameraView
{
	GENERATED_BODY()

	UPROPERTY()
	FVector PivotLocation = FVector::ZeroVector;

	UPROPERTY()
	float TargetArmLength = 800.f;

	UPROPERTY()
	float FOV = 90.f;

	UPROPERTY()
	FVector SocketOffset = FVector::ZeroVector;

	UPROPERTY()
	float LocationInterpSpeed = 8.f;

	UPROPERTY()
	float ZoomInterpSpeed = 5.f;

	UPROPERTY()
	float FOVInterpSpeed = 5.f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYCameraComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGYCameraComponent(const FObjectInitializer& ObjectInitializer);

	static const FName NAME_ActorFeatureName;

	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	                                FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	                                   FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	// 인터페이스 함수 끝 --
protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> CameraComponent;

	// 현재 카메라 모드
	UPROPERTY(Transient)
	TObjectPtr<class UGYCameraModeBase> CurrentCameraMode;

	//데이터 맵
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UGYCameraModeData>> CameraModeMap;

	//현재 뷰
	FGYCameraView CurrentView;

public:
	//기본 카메라 모드
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	TObjectPtr<UGYCameraModeData> DefaultCameraMode;
	// 카메라 모드 데이터 에셋
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	TArray<TObjectPtr<UGYCameraModeData>> CameraModeAssets;

private:
	bool bInitializedView = false;

protected:
	//카메라 데이터 캐싱
	void InitializeCameraModes();
	//태그 변경 감지
	void RegisterCameraTagEvents();
	//모드 결정
	void ResolveCameraMode();
	//모드 변경
	void SetCameraMode(UGYCameraModeData* NewModeData);
	//뷰 적용
	void ApplyCameraView(const FGYCameraView& View) const;
	//ASC 접근
	UGYAbilitySystemComponent* GetAbilitySystemComponent() const;

	void OnCameraTagChanged(FGameplayTag Tag, int32 NewCount);

public:
	/* 카메라 이펙트 */
	void PushCameraEffect(const FGYCameraEffectContext& Context);
private:
	// 활성된 카메라 효과
	UPROPERTY()
	TArray<TObjectPtr<UGYCameraEffectBase>> ActiveEffects;

	UPROPERTY(EditDefaultsOnly)
	TMap<EGYCameraEffectType, TSubclassOf<UGYCameraEffectBase>> CameraEffectMap;
};
