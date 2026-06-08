#include "Quest/QuestTriggerVolume.h"

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

	BoxComponent->OnComponentBeginOverlap.AddDynamic(
		this,
		&AQuestTriggerVolume::OnMeshBeginOverlap);
}

void AQuestTriggerVolume::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                             const FHitResult& SweepResult)
{
	if (bTriggered)
		return;
	// 대화
	GY_LOG(Content, CYS, "NPC와의 대화");
	PlayNarrativeDialogue();
	// 퀘스트 활성화
	UQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	QuestSubsystem->StartQuest(QuestTag);
	GY_LOG(Content, CYS, "퀘스트 활성화:%s", *(QuestTag.ToString()));
	bTriggered = true;
}

void AQuestTriggerVolume::PlayNarrativeDialogue() const
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->BroadcastNarrativeDialogue(QuestTag);
	}
}

UQuestSubsystem* AQuestTriggerVolume::GetQuestSubsystem() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UQuestSubsystem>();
	}
	return nullptr;
}
