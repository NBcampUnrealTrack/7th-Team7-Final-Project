#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
#include "GYTestCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAbilitySet;
class UInputMappingContext;
class UInputAction;
class AGYPlayerState;
struct FInputActionValue;

UCLASS(Blueprintable)
class GY_API AGYTestCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGYTestCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Abilities")
	TObjectPtr<UAbilitySet> TestAbilitySet;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Input")
	TObjectPtr<UInputAction> ParryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Stats")
	float InitialHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Test|Attack")
	float HoldToChargeTime = 1.0f;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void InitGAS();
	void OnMove(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	void OnAttackReleased(const FInputActionValue& Value);
	void OnHoldToChargeThreshold();
	void OnParry(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	FAbilitySetGrantedHandles AbilitySetHandles;
	FTimerHandle HoldToChargeTimer;
};
