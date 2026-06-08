#include "World/VolumeActor/GYRegionVolume.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Loot/RegionLootData.h"
#include "World/ActorManagement/GYWorldDataSettings.h"

AGYRegionVolume::AGYRegionVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AGYRegionVolume::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGYRegionVolume::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AGYRegionVolume::OnOverlapEnd);

	// 0.1초 뒤에 체크하여 초기화 시간 확보
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
	{
		if (!IsValid(this)) return;

		TArray<AActor*> Overlapping;
		TriggerBox->GetOverlappingActors(Overlapping, APawn::StaticClass());
		for (AActor* Actor : Overlapping)
		{
		   OnOverlapBegin(TriggerBox, Actor, nullptr, 0, false, FHitResult());
		}
	}, 0.1f, false);
}

bool AGYRegionVolume::IsLocationInside(const FVector& WorldLocation) const
{
	if (!IsValid(TriggerBox)) return false;

	const FVector Local = TriggerBox->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = TriggerBox->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X
		&& FMath::Abs(Local.Y) <= Extent.Y
		&& FMath::Abs(Local.Z) <= Extent.Z;
}

void AGYRegionVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	if (Region->RegionId.IsValid()) // 지역 태그 갱신
	{
		if (UAbilitySystemComponent* ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			FGameplayTag RegionParentTag = FGameplayTag::RequestGameplayTag(TEXT("Region"));
			FGameplayTagContainer OwnedTags;

			ASC->GetOwnedGameplayTags(OwnedTags);
			FGameplayTagContainer TagsToRemove = OwnedTags.Filter(FGameplayTagContainer(RegionParentTag));

			ASC->RemoveLooseGameplayTags(TagsToRemove);
			ASC->AddLooseGameplayTag(Region->RegionId);
		}
	}

	// UI 배너용 메시지
	FGYRegionEnteredMessage Msg;
	Msg.RegionId = Region->RegionId;
	Msg.RegionDisplayName = Region->RegionDisplayName;
	Msg.RegionLevel = GetDefault<UGYWorldDataSettings>()->DefaultRegionLevel;
	Msg.RegionIcon = Region->RegionIcon;
	Msg.BossActor = TargetBossActor;
	Msg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Entered, Msg);
}

void AGYRegionVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;
	if (TriggerBox->IsOverlappingActor(Pawn))
	{
		return;
	}

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	FGYRegionExitedMessage ExitMsg;
	ExitMsg.RegionId = Region->RegionId;
	ExitMsg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Exited, ExitMsg);
}
