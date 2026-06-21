#pragma once

#include "CoreMinimal.h"
#include "ParriedEventContext.generated.h"

USTRUCT()
struct FParriedEventContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	FName SourceHitBone;

	virtual UScriptStruct* GetScriptStruct() const override { return FParriedEventContext::StaticStruct(); }
	virtual FGameplayEffectContext* Duplicate() const override { return new FParriedEventContext(*this); }
};
