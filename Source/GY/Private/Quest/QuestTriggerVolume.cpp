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

	BoxComponent->OnComponentBeginOverlap.AddDynamic(
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
		if (!bTriggered)
		{
			bTriggered = true;
			GetQuestSubsystem()->StartQuest(QuestTag);
			GY_LOG(Content, CYS, "퀘스트 활성화:%s", *QuestTag.ToString());
		}
		return;
	}

	// 클라이언트: 자신의 캐릭터만, 각 플레이어 독립적으로 한 번
	if (!Character->IsLocallyControlled()) return;
	if (bTriggered) return;

	bTriggered = true;
	GY_LOG(Content, CYS, "NPC와의 대화");
	PlayNarrativeDialogue();
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
