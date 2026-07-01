#include "Enemy/AnimNotify/AnimNotify_EnemyComboCancle.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/GYCharacter.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"


void UAnimNotify_EnemyComboCancle::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	if (!AIC) return;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return;

	UGYEnemyAttackAbilityBase* Ability = Cast<UGYEnemyAttackAbilityBase>(BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!CancelExecution(MeshComp))
	{
		ASC->CancelAbility(Ability);
	}
}

FString UAnimNotify_EnemyComboCancle::GetNotifyName_Implementation() const
{
	return TEXT("Enemy Combo Cancle");
}

bool UAnimNotify_EnemyComboCancle::CancelExecution(USkeletalMeshComponent* MeshComp)
{
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return false;

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	if (!AIC) return false;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return false;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return false;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!TargetActor) return false;

	FVector ForwardXY = Enemy->GetActorForwardVector();
	ForwardXY.Z = 0.f;
	ForwardXY.Normalize();

	FVector ToTargetXY = TargetActor->GetActorLocation() - Enemy->GetActorLocation();
	ToTargetXY.Z = 0.f;
	ToTargetXY.Normalize();

	const float Dot = FVector::DotProduct(ForwardXY, ToTargetXY);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

	return AngleDeg <= (BB->GetValueAsFloat(EnemyBBKeys::AttackAngle));
}
