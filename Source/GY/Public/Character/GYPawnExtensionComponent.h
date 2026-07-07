// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "Engine/TimerHandle.h"
#include "GYPawnExtensionComponent.generated.h"


class APawn;
class UGYAbilitySystemComponent;
class UGYPawnData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGYPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	static const FName NAME_ActorFeatureName;

	// PawnData는 PlayerState에서 옴(GameMode가 Experience로 set). 컴포넌트는 접근자만 제공.
	UFUNCTION(BlueprintPure, Category = "GY|PawnData")
	const UGYPawnData* GetPawnData() const;

	// -- IGameFramework~ 인터페이스 구현부 override
	//기능 이름 반환
	virtual FName GetFeatureName() const override {return NAME_ActorFeatureName;};
	// 상태 변환이 가능한지 조건 검사
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;

	// 상태가 변경되었을 때 실행할 실제 동작
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager,
									   FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	// 다른 액터의 상태가 변했을 때 나도 연쇄적으로 상태를 바꿀지 검사
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	// 초기화 진행을 시도하는 핵심 함수
	virtual void CheckDefaultInitialization() override;
	// -- 끝 --

	// init에 필요한 복제값이 늦게 도착한 외부(PC/PS 등)에서, 해당 폰의 초기화 체인을 다시 검사하도록 요청한다.
	// PawnExtension에서 검사를 다시 돌리면 같은 폰의 다른 init 컴포넌트(HeroComponent 등)도 함께 검사된다.
	static void RequestInitStateRecheck(APawn* Pawn);

protected:
	//컴포넌트 생명주기 함수
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 일정 시간 내 초기화가 GameplayReady까지 못 가면(=복제값 누락 등으로 조용히 멈춤) 어디서 막혔는지 경고로 노출한다.
	void OnInitWatchdog();

	TWeakObjectPtr<UGYAbilitySystemComponent> CachedASC;

	FTimerHandle InitWatchdogTimer;

	// 0 이하면 워치독 비활성. 정상 로딩이 길어도 오탐하지 않도록 넉넉히 잡는다.
	UPROPERTY(EditDefaultsOnly, Category = "GY|Init")
	float InitWatchdogSeconds = 20.f;
};
