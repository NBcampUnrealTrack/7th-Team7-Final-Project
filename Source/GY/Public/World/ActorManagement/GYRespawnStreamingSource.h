#pragma once

#include "CoreMinimal.h"
#include "WorldPartition/WorldPartitionStreamingSource.h"

class FGYRespawnStreamingSource : public IWorldPartitionStreamingSourceProvider
{
public:
	FGYRespawnStreamingSource(const FVector& InLocation, const FRotator& InRotation)
		: Location(InLocation)
		, Rotation(InRotation)
	{
	}

	virtual bool GetStreamingSource(FWorldPartitionStreamingSource& OutStreamingSource) const override
	{
		OutStreamingSource = FWorldPartitionStreamingSource(
			FName(TEXT("GYRespawnPrewarm")),
			Location,
			Rotation,
			EStreamingSourceTargetState::Activated,
			true,
			EStreamingSourcePriority::Highest,
			false);
		return true;
	}

private:
	FVector Location;
	FRotator Rotation;
};
