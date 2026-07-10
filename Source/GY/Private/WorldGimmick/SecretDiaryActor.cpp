#include "WorldGimmick/SecretDiaryActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/BoxComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Logging/GYLogManager.h"
#include "Quest/QuestSubsystem.h"
#include "TimerManager.h"


ASecretDiaryActor::ASecretDiaryActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetCollisionProfileName(GYCollisionProfile::Interactable);
	InteractionBox->SetupAttachment(Root);

	InteractTag = GYGameplayTags::Interaction_SecretDiary;
	InteractionText = NSLOCTEXT("SecretDiary", "Default", "줍기");
}

void ASecretDiaryActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	if (!Interactor) return;

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.OptionTag = InteractTag;
	Option.SourceObject = const_cast<ASecretDiaryActor*>(this);
	OutOptions.Add(Option);
}

void ASecretDiaryActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (!HasAuthority()) return;
	if (DiaryTags.IsEmpty()) return;

	if (const UGameInstance* GI = GetGameInstance())
	{
		UQuestSubsystem* QS = GI->GetSubsystem<UQuestSubsystem>();
		if (!QS) return;
		if (!QS->ArePrerequisitesMet(DiaryTags[0]))
			return; // 선행 퀘스트 완료 체크

		for (const auto Tag : DiaryTags)
		{
			QS->StartQuest(Tag);
			GY_LOG(Content, CYS, "기록 일지 활성화: %s", *Tag.ToString());
		}
	}
	// 이펙트
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Interactor);
	if (!ASC) return;

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = Interactor;
	CueParameters.Location = GetActorLocation();
	ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Interaction_SecretDiary, CueParameters);

	// 1초 뒤 파괴
	if (bDestroyPending) return;
	bDestroyPending = true;

	TWeakObjectPtr<ASecretDiaryActor> WeakThis(this);
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (ASecretDiaryActor* StrongThis = WeakThis.Get())
		{
			StrongThis->Destroy();
		}
	}), 1.0f, false);
}
