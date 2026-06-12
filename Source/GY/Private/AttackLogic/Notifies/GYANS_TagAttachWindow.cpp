#include "AttackLogic/Notifies/GYANS_TagAttachWindow.h"
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

void UGYANS_TagAttachWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = GetASC(MeshComp);
	if (ASC && !Tags.IsEmpty())
	{
		ASC->AddLooseGameplayTags(Tags);
	}
}

void UGYANS_TagAttachWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = GetASC(MeshComp);
	if (ASC && !Tags.IsEmpty())
	{
		ASC->RemoveLooseGameplayTags(Tags);
	}
}

FString UGYANS_TagAttachWindow::GetNotifyName_Implementation() const
{
	return !Tags.IsEmpty()
		? FString::Printf(TEXT("TagAttachWindow [%s]"), *Tags.ToString())
		: TEXT("TagAttachWindow");
}
