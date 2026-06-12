#include "Enemy/Abilities/ShockWaveAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

UShockWaveAbility::UShockWaveAbility()
{
	FRichCurve* Curve = DamageFalloffCurve.GetRichCurve();
	Curve->AddKey(0.f, 1.f);
	Curve->AddKey(1.f,0.2f);
}

void UShockWaveAbility::ExecuteShockWave()
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* BossActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	if (!SourceASC || !BossActor) return;

	const FVector BossLoc = BossActor->GetActorLocation();

	if (ShockWaveCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = BossLoc;
		Params.SourceObject = BossActor;
		SourceASC->ExecuteGameplayCue(ShockWaveCueTag, Params);
	}

	float BaseMul = 1.f;
	float BaseAdd = 0.f;
	float BaseStagger = 0.f;
	float BaseStun = 0.f;
	if (HitDamageWeights.IsValidIndex(0))
	{
		const FHitDamageWeight& Weight = HitDamageWeights[0];
		BaseMul = Weight.Multiplicative;
		BaseAdd = Weight.Additive;
		BaseStagger = Weight.Stagger;
		BaseStun = Weight.Stun;
	}

	UWorld* World = BossActor->GetWorld();
	if (!World) return;

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn || Pawn == BossActor) continue;;
		if (!Pawn->IsPlayerControlled()) continue;

		const float Distance = FVector::Dist(BossLoc, Pawn->GetActorLocation());
		if (Distance > MaxRadius) continue;

		const float NormalizeDist = FMath::Clamp(Distance / MaxRadius, 0.f, 1.f);
		const FRichCurve* Curve = DamageFalloffCurve.GetRichCurveConst();
		const float Falloff = Curve ? Curve->Eval(NormalizeDist, 1.f) : 1.f;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (!TargetASC) continue;

		FGYHitContext HitContext;
		HitContext.SourceASC = SourceASC;
		HitContext.TargetASC = TargetASC;
		HitContext.MotionMultiplier = BaseMul * Falloff;
		HitContext.Additive = BaseAdd * Falloff;
		HitContext.StaggerAmount = BaseStagger * Falloff;
		HitContext.StunAmount = BaseStun * Falloff;

		UGYCombatStatics::ApplyHitImpact(HitContext);
	}
}
