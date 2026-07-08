#include "Quest/QuestTriggerVolume.h"

#include "Character/GYCharacter.h"
#include "Components/BoxComponent.h"
#include "Logging/GYLogManager.h"
#include "Quest/QuestSubsystem.h"


AQuestTriggerVolume::AQuestTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));

	RootComponent = BoxComponent;

	BoxComponent->SetGenerateOverlapEvents(true);

	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AQuestTriggerVolume::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&AQuestTriggerVolume::OnMeshBeginOverlap);
}

void AQuestTriggerVolume::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                             const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->IsA(AGYCharacter::StaticClass())) return;

	AGYCharacter* Character = Cast<AGYCharacter>(OtherActor);
	if (!IsValid(Character)) return;

	// 서버: 퀘스트 시작만 (한 번)
	if (HasAuthority())
	{
		UQuestSubsystem* QS = GetQuestSubsystem();
		if (!QS) return;
		if (QuestTags.IsEmpty())
		{
			GY_ERROR(Content,CYS,"퀘스트 트리거 볼륨 태그 빠짐. 에디터 수정 필수");
			return;
		}
		if (!QS->ArePrerequisitesMet(QuestTags[0]))
			return; // 선행 퀘스트 완료 체크
		if (!bQuestStarted)
		{
			bQuestStarted = true;
			QS->StartQuest(QuestTags[0]);
			GY_LOG(Content, CYS, "퀘스트 활성화:%s", *QuestTags[0].ToString());
		}
	}

	// 클라이언트: 자신의 캐릭터만, 각 플레이어 독립적으로 한 번
	if (!Character->IsLocallyControlled()) return;
	if (bDialoguePlayed && !bIsLoop) return;

	const FGameplayTag DialogueTag = GetRandomQuestTag();

	// Quest 태그는 선행 퀘스트 완료 체크, Dialogue 태그는 체크 없이 재생
	static const FGameplayTag QuestRootTag = FGameplayTag::RequestGameplayTag(FName("Quest"));
	if (DialogueTag.MatchesTag(QuestRootTag))
	{
		UQuestSubsystem* QS = GetQuestSubsystem();
		if (!QS || !QS->ArePrerequisitesMet(DialogueTag))
			return;
	}

	bDialoguePlayed = true;
	GY_LOG(Content, CYS, "NPC와의 대화");
	PlayNarrativeDialogue(GetRandomQuestTag());
}

void AQuestTriggerVolume::PlayNarrativeDialogue(FGameplayTag DialogueTag) const
{
	if (!DialogueTag.IsValid())
	{
		return;
	}
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->BroadcastNarrativeDialogue(DialogueTag);
	}
}

FGameplayTag AQuestTriggerVolume::GetRandomQuestTag() const
{
	if (QuestTags.IsEmpty())
	{
		return FGameplayTag();
	}

	const int32 RandomIndex = FMath::RandRange(0, QuestTags.Num() - 1);
	GY_LOG(Content, CYS, "다이얼로그: ", QuestTags[RandomIndex]);
	return QuestTags[RandomIndex];
}

UQuestSubsystem* AQuestTriggerVolume::GetQuestSubsystem() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UQuestSubsystem>();
	}
	return nullptr;
}
