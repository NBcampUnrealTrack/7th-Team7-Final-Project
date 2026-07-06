#include "Enemy/Actor/GYWeaponHitBox.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "Core/GameplayTags/EventTags.h"

UGYWeaponHitBox::UGYWeaponHitBox()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(false);
}

void UGYWeaponHitBox::BeginPlay()
{
	Super::BeginPlay();
	OnComponentBeginOverlap.AddDynamic(this, &UGYWeaponHitBox::OnBeginOverlap);
}

void UGYWeaponHitBox::BeginHitDetection(AActor* InSource)
{
	SourceActor = InSource;
	HitActors.Reset();
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetGenerateOverlapEvents(true);
}

void UGYWeaponHitBox::EndHitDetection()
{
	SetGenerateOverlapEvents(false);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitActors.Reset();
}

void UGYWeaponHitBox::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	AActor* Source = SourceActor.Get();
	if (!Source || !Other) return;
	if (Other == Source || Other == Source->GetOwner()) return; // 자기 자신/스폰 오너 무시
	if (HitActors.Contains(Other)) return;
	HitActors.Add(Other);

	// 순수 Overlap(bFromSweep=false)은 HitResult가 비므로 근사값으로 채운다.
	const FHitResult Hit = bFromSweep
		? Sweep
		: FHitResult(Other, OtherComp, Other->GetActorLocation(),
			(Other->GetActorLocation() - GetComponentLocation()).GetSafeNormal());

	// 기존 EnemyWeaponTrace 와 동일한 페이로드 → 어빌리티 OnWeaponHit 재사용
	FGameplayAbilityTargetData_SingleTargetHit* TargetData =
		new FGameplayAbilityTargetData_SingleTargetHit(Hit);
	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(TargetData);

	FGameplayEventData Payload;
	Payload.Instigator = Source;
	Payload.Target = Other;
	Payload.TargetData = TargetDataHandle;

	FParriedEventContext* Ctx = new FParriedEventContext();
	Ctx->SourceHitBone = HitBoneName;
	Payload.ContextHandle = FGameplayEffectContextHandle(Ctx);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Source, GYGameplayTags::Event_Enemy_WeaponTrace_Hit, Payload);

}





