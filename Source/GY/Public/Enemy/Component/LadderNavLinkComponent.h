#pragma once

#include "CoreMinimal.h"
#include "NavLinkCustomComponent.h"
#include "Character/GYCharacterMovementComponent.h"
#include "LadderNavLinkComponent.generated.h"


class UPathFollowingComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API ULadderNavLinkComponent : public UNavLinkCustomComponent
{
	GENERATED_BODY()

public:
	virtual bool OnLinkMoveStarted(class UObject* PathComp, const FVector& DestPoint) override;

protected:
	UFUNCTION()
	void HandleClimbEnded(ELadderExitReason Reason);

	TWeakObjectPtr<UPathFollowingComponent> PathFollowingComponent;
	TWeakObjectPtr<UGYCharacterMovementComponent> CharacterMovementComponent;

};
