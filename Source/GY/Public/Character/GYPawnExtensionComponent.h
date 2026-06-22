// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GYPawnExtensionComponent.generated.h"


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

protected:
	//컴포넌트 생명주기 함수
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// PawnData의 AbilitySet 부여 핸들. EndPlay에서 ASC로부터 회수한다.
	FAbilitySetGrantedHandles GrantedHandles;
	TWeakObjectPtr<UGYAbilitySystemComponent> CachedASC;
};
