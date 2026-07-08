#include "Camera/CameraVolumeActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/GYCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Logging/GYLogManager.h"


ACameraVolumeActor::ACameraVolumeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));

	RootComponent = BoxComponent;

	BoxComponent->SetGenerateOverlapEvents(true);

	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void ACameraVolumeActor::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&ACameraVolumeActor::OnMeshBeginOverlap);

	BoxComponent->OnComponentEndOverlap.AddUniqueDynamic(
		this,
		&ACameraVolumeActor::OnMeshEndOverlap);
}

void ACameraVolumeActor::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                            const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!ASC)
	{
		return;
	}
	ASC->AddLooseGameplayTag(CameraTag);
	GY_LOG(Player, CYS, "카메라 볼륨 입장 - %s", *CameraTag.ToString());
	if (TargetActor)
	{
		if (APawn* Pawn = Cast<APawn>(OtherActor))
		{
			if (UGYCameraComponent* CameraComp =
				Pawn->FindComponentByClass<UGYCameraComponent>())
			{
				CameraComp->SetCameraTargetActor(TargetActor);
			}
		}
	}
}

void ACameraVolumeActor::OnMeshEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!ASC)
	{
		return;
	}
	ASC->RemoveLooseGameplayTag(CameraTag);
	GY_LOG(Player, CYS, "카메라 볼륨 퇴장 - %s", *CameraTag.ToString());
	if (TargetActor)
	{
		if (APawn* Pawn = Cast<APawn>(OtherActor))
		{
			if (UGYCameraComponent* CameraComp =
				Pawn->FindComponentByClass<UGYCameraComponent>())
			{
				CameraComp->SetCameraTargetActor(nullptr);
			}
		}
	}
}
