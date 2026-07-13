#include "Camera/CameraVolumeActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/GYCameraComponent.h"
#include "Character/GYCharacter.h"
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
	if (const AGYCharacter* Character = Cast<AGYCharacter>(OtherActor); Character && Character->IsDead())
	{
		// 사망 후 래그돌 물리 흔들림으로 볼륨과 재오버랩되어 카메라 태그가 재부착되는 것을 방지
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
