#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GYCharacter.generated.h"

class UGYPawnExtensionComponent;
class UAbilitySystemComponent;
class UActiveEquipmentComponent;

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


	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
protected:
	// 컴포넌트 매니저 통신을 위한 생명주기 함수 오버라이드
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UActiveEquipmentComponent> ActiveEquipmentComponent;
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HB|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGYPawnExtensionComponent> PawnExtComponent;


};
