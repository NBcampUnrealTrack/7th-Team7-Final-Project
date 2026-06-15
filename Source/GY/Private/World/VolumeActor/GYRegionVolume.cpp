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
#include "GameFramework/PlayerController.h"

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &AGYRegionVolume::ProcessInitialOverlappingPawns));
	}
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
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandlePawnEntered(Cast<APawn>(OtherActor));
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
	HandlePawnExited(Pawn);
}

void AGYRegionVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYRegionVolume, TargetBossActor); // 서버에서 복제한 액터 클라이언트로 동기화
}

void AGYRegionVolume::HandlePawnEntered(APawn* Pawn)
{
	if (!Pawn) return;

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	if (Region->RegionId.IsValid()) // 지역 태그 갱신
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			FGameplayTag RegionParentTag = FGameplayTag::RequestGameplayTag(TEXT("Region"));
			FGameplayTagContainer OwnedTags;

			ASC->GetOwnedGameplayTags(OwnedTags);
			FGameplayTagContainer TagsToRemove = OwnedTags.Filter(FGameplayTagContainer(RegionParentTag));

			ASC->RemoveLooseGameplayTags(TagsToRemove);
			ASC->AddLooseGameplayTag(Region->RegionId);
		}
	}
	if (GetNetMode() == NM_DedicatedServer) return;

	FGYRegionEnteredMessage Msg;
	Msg.RegionId = Region->RegionId;
	Msg.RegionDisplayName = Region->RegionDisplayName;
	Msg.RegionLevel = GetDefault<UGYWorldDataSettings>()->DefaultRegionLevel;
	Msg.RegionIcon = Region->RegionIcon;
	Msg.BossActor = TargetBossActor;
	Msg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Entered, Msg);
}

void AGYRegionVolume::HandlePawnExited(APawn* Pawn)
{
	if (!Pawn) return;

	URegionLootData* Region = RegionData.LoadSynchronous();
	if (!Region) return;

	// 태그 제거
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		if (!ASC->HasMatchingGameplayTag(Region->RegionId)) return;
		ASC->RemoveLooseGameplayTag(Region->RegionId);
	}

	if (GetNetMode() == NM_DedicatedServer) return;

	FGYRegionExitedMessage ExitMsg;
	ExitMsg.RegionId = Region->RegionId;
	ExitMsg.Pawn = Pawn;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(GYGameplayTags::Message_Region_Exited, ExitMsg);
}

void AGYRegionVolume::ProcessInitialOverlappingPawns()
{
	if (!IsValid(TriggerBox)) return;

	TArray<AActor*> Overlapping;
	TriggerBox->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping) // 오버랩된 폰 - 지역 진입 처리
	{
		HandlePawnEntered(Cast<APawn>(Actor));
	}

	if (GetNetMode() == NM_Client)
	{
		TryNotifyLocalPawn();
	}
}

void AGYRegionVolume::TryNotifyLocalPawn()
{
	if (!IsValid(TriggerBox)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* LocalPawn = PC ? PC->GetPawn() : nullptr;

	if (LocalPawn && TriggerBox->IsOverlappingActor(LocalPawn))
	{
		HandlePawnEntered(LocalPawn);
		return;
	}

	// 폰이 아직 없거나 영역 밖이면 잠시 후 재시도
	if (LocalPawnRetryCount < 10)
	{
		++LocalPawnRetryCount;
		World->GetTimerManager().SetTimer(LocalPawnRetryTimer, FTimerDelegate::CreateUObject(
			this, &AGYRegionVolume::TryNotifyLocalPawn), 0.5f, false);
	}
}
