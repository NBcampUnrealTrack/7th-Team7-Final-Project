#include "Enemy/Actor/TentacleActor.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ATentacleActor::ATentacleActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SetRootComponent(SkeletalMesh);

	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
	SkeletalMesh->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void ATentacleActor::BeginPlay()
{
	Super::BeginPlay();

	CachedWeaponTraceSockets();

	if (ActiveMontage && SkeletalMesh)
	{
		if (UAnimInstance* AI = SkeletalMesh->GetAnimInstance())
		{
			if (!AI->Montage_IsPlaying(ActiveMontage))
			{
				AI->Montage_Play(ActiveMontage, ActiveMontagePlayRate);
			}
		}
	}
}

void ATentacleActor::CachedWeaponTraceSockets()
{
	WeaponTraceSockets.Empty();
	if (!SkeletalMesh) return;

	TArray<FName> Found;
	for (const FName& SocketName : SkeletalMesh->GetAllSocketNames())
	{
		if (SocketName.ToString().StartsWith(WeaponTraceBonePrefix))
		{
			Found.Add(SocketName);
		}
	}

	Found.Sort([](const FName& A, const FName& B)
	{
		return A.ToString() < B.ToString();
	});

	WeaponTraceSockets = Found;
}

void ATentacleActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATentacleActor, ActiveMontage);
	DOREPLIFETIME(ATentacleActor, ActiveMontagePlayRate);
}

UAbilitySystemComponent* ATentacleActor::GetAbilitySystemComponent() const
{
	// 스폰 시 Owner = Boss 로 세팅되어 있어야 함.
	if (const IAbilitySystemInterface* OwnerASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return OwnerASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

void ATentacleActor::StartMontagePlayback(UAnimMontage* Montage, float Rate)
{
	if (!HasAuthority() || !Montage) return;

	ActiveMontage = Montage;
	ActiveMontagePlayRate = Rate;
	// 서버 측 Montage_Play 는 어빌리티의 PlayMontageOnMesh 태스크가 담당.
	// 클라이언트는 OnRep_ActiveMontage 에서 자체 재생.
}

void ATentacleActor::OnRep_ActiveMontage()
{
	if (!ActiveMontage || !SkeletalMesh) return;
	if (UAnimInstance* AI = SkeletalMesh->GetAnimInstance())
	{
		AI->Montage_Play(ActiveMontage, ActiveMontagePlayRate);
	}
}

void ATentacleActor::StartFadeOutAndDestroy(float FadeDuration)
{
	if (FadeDuration <= 0.f)
	{
		Destroy();
		return;
	}

	GetWorldTimerManager().SetTimer(
		FadeTimerHandle, this, &ATentacleActor::HandleFadeFinished,
		FadeDuration, false);
}

void ATentacleActor::HandleFadeFinished()
{
	Destroy();
}
