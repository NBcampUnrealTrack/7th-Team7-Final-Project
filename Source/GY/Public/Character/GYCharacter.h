#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GYCharacter.generated.h"

class UGYPawnExtensionComponent;
class UAbilitySystemComponent;
class UActiveEquipmentComponent;
class UInteractionComponent;

UCLASS()
class GY_API AGYCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGYCharacter();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;




	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure)
	UActiveEquipmentComponent* GetActiveEquipmentComponent() const { return ActiveEquipmentComponent; }

	UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }


	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable)
	void Server_SetFacingYaw(float Yaw);
protected:
	// 컴포넌트 매니저 통신을 위한 생명주기 함수 오버라이드
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UActiveEquipmentComponent> ActiveEquipmentComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInteractionComponent> InteractionComponent;

	/** 캐릭터 초기화 완료 방송 */
	void BroadcastCharacterReady();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HB|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGYPawnExtensionComponent> PawnExtComponent;

public:
	UPROPERTY()
	TWeakObjectPtr<AActor> Target;
	//TODO::타겟을 정해주는 로직 필요
};
