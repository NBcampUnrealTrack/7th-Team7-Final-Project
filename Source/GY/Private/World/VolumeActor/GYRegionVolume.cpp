#include "World/VolumeActor/GYRegionVolume.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Loot/RegionLootData.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "Net/UnrealNetwork.h"

AGYRegionVolume::AGYRegionVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AGYRegionVolume::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGYRegionVolume::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AGYRegionVolume::OnOverlapEnd);
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

	if (!IsValid(TargetBossActor)) return; // Actor 유효성 체크

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	if (Region->RegionId.IsValid()) // 지역 태그 갱신
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
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
	if (TriggerBox->IsOverlappingActor(Pawn)) // 경계 걸친 경우 오작동 방지
	{
		return;
	}

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (ASC)
	{
		if (!ASC->HasMatchingGameplayTag(Region->RegionId)) return;
		ASC->RemoveLooseGameplayTag(Region->RegionId); // 태그 회수
	}
	FGYRegionExitedMessage ExitMsg;
	ExitMsg.RegionId = Region->RegionId;
	ExitMsg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Exited, ExitMsg);
}

void AGYRegionVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYRegionVolume, TargetBossActor); // 서버에서 복제한 액터 클라이언트로 동기화
}
