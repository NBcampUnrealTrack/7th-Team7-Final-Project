#include "Enemy/Abilities/AreaDenialAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UAreaDenialAbility::UAreaDenialAbility()
{
}

bool UAreaDenialAbility::IsSpacingOK(const FVector& Candidate, const TArray<FVector>& Placed) const
{
	for (const FVector& Place : Placed)
	{
		if (FVector::Dist(Candidate, Place) < MinSpacing)
			return false;
	}
	return true;
}

void UAreaDenialAbility::ExecuteAreaDenail()
{
	UWorld* World = GetWorld();
	if (!World || !HazardActorClass) return;

	AActor* BossActor = GetAvatarActorFromActorInfo();
	const FVector Origin = BossActor ? BossActor->GetActorLocation() : FVector::ZeroVector;

	TArray<FVector> PlayerLocations;
	if (PlacementMode == EHazardPlacementMode::AroundPlayers)
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Pawn = *It;
			if (Pawn && Pawn->IsPlayerControlled())
			{
				PlayerLocations.Add(Pawn->GetActorLocation());
			}
		}
	}

	TArray<FVector> Placed;
	Placed.Reserve(HazardCount);
	const int32 MaxAttempts = HazardCount * 10;
	int32 Attempts = 0;
	int32 Spawned = 0;

	while (Spawned < HazardCount && Attempts < MaxAttempts)
	{
		++Attempts;
		FVector Candidate = FVector::ZeroVector;

		switch (PlacementMode)
		{
		case EHazardPlacementMode::RandomInArea:
			{
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				const float Dist = FMath::FRandRange(0.f, ArenaRadius);
				Candidate = Origin + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
			}
			break;
		case EHazardPlacementMode::AroundBoss:
			{
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				Candidate = Origin + FVector(FMath::Cos(Angle) * PlacementRadius,
					FMath::Sin(Angle) * PlacementRadius, 0.f);
			}
			break;
		case EHazardPlacementMode::AroundPlayers:
			{
				if (PlayerLocations.Num() == 0) return;
				const FVector& PLoc = PlayerLocations[FMath::RandRange(0, PlayerLocations.Num() - 1)];
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				const float Dist = FMath::FRandRange(0.f, PlacementRadius);
				Candidate = PLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
			}
			break;
		}

		if (!IsSpacingOK(Candidate, Placed)) continue;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.Owner = BossActor;

		if (World->SpawnActor<AActor>(HazardActorClass, Candidate, FRotator::ZeroRotator, Params))
		{
			Placed.Add(Candidate);
			++Spawned;
		}
	}
}


