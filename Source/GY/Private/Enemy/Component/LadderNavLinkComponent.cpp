#include "Enemy/Component/LadderNavLinkComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "WorldGimmick/Ladder.h"


bool ULadderNavLinkComponent::OnLinkMoveStarted(class UObject* PathComp, const FVector& DestPoint)
{
	Super::OnLinkMoveStarted(PathComp, DestPoint);

	ALadder* Ladder = Cast<ALadder>(GetOwner());
	UPathFollowingComponent* PFC = Cast<UPathFollowingComponent>(PathComp);
	if (!Ladder || !PFC || !Ladder->CanClimb())
	{
		if (PFC)
		{
			PFC->ResumeMove();
		}
		return false;
	}

	APawn* AgentPawn = nullptr;
	if (AAIController* AICon = Cast<AAIController>(PFC->GetOwner()))
	{
		AgentPawn = AICon->GetPawn();
	}
	else if (APawn* DirectPawn = Cast<APawn>(PFC->GetOwner()))
	{
		AgentPawn = DirectPawn;
	}
	if (!AgentPawn)
	{
		PFC->ResumeMove();
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AgentPawn);
	if (!ASC)
	{
		PFC->ResumeMove();
		return false;
	}

	const float AgentZ = AgentPawn->GetActorLocation().Z;
	const float LadderTopZ = Ladder->GetActorLocation().Z + Ladder->GetClimbDistance();
	const bool bFromTop = AgentZ > (LadderTopZ - 50.f);

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Ladder_ClimbRequest;
	Payload.Instigator = AgentPawn;
	Payload.OptionalObject = Ladder;
	Payload.EventMagnitude = bFromTop ? 1.f : 0.f;

	ASC->HandleGameplayEvent(GYGameplayTags::Event_Ladder_ClimbRequest, &Payload);

	PathFollowingComponent = PFC;

	if (ACharacter* AgentChar = Cast<ACharacter>(AgentPawn))
	{
		if (UGYCharacterMovementComponent* CMC = Cast<UGYCharacterMovementComponent>(AgentChar->GetCharacterMovement()))
		{
			CMC->OnClimbingEnded.AddUniqueDynamic(this, &ULadderNavLinkComponent::HandleClimbEnded);
			CharacterMovementComponent = CMC;
		}
	}
	return true;
}

void ULadderNavLinkComponent::HandleClimbEnded(ELadderExitReason Reason)
{
	if (CharacterMovementComponent.IsValid())
	{
		CharacterMovementComponent->OnClimbingEnded.RemoveDynamic(this, &ULadderNavLinkComponent::HandleClimbEnded);
		CharacterMovementComponent.Reset();
	}

	if (PathFollowingComponent.IsValid())
	{
		PathFollowingComponent->ResumeMove();
		PathFollowingComponent.Reset();
	}
}
