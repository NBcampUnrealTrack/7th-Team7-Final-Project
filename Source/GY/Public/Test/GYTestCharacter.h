#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GYTestCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS(Blueprintable)
class GY_API AGYTestCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGYTestCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ParryAction;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void OnMove(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	void OnAttackReleased(const FInputActionValue& Value);
	void OnParry(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;
};
