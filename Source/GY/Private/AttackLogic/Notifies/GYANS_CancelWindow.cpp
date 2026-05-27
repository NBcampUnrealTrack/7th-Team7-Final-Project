#include "AttackLogic/Notifies/GYANS_CancelWindow.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

namespace
{
	UAbilitySystemComponent* GetASC(USkeletalMeshComponent* MeshComp)
	{
		AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (!Owner) return nullptr;
		const IAbilitySystemInterface* ABSI = Cast<IAbilitySystemInterface>(Owner);
		return ABSI ? ABSI->GetAbilitySystemComponent() : nullptr;
	}
}

void UGYANS_CancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = GetASC(MeshComp);
	if (ASC && WindowTag.IsValid())
	{
		ASC->AddLooseGameplayTag(WindowTag);
	}
}

void UGYANS_CancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = GetASC(MeshComp);
	if (ASC && WindowTag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(WindowTag);
	}
}

FString UGYANS_CancelWindow::GetNotifyName_Implementation() const
{
	return WindowTag.IsValid()
		? FString::Printf(TEXT("CancelWindow [%s]"), *WindowTag.ToString())
		: TEXT("CancelWindow");
}
