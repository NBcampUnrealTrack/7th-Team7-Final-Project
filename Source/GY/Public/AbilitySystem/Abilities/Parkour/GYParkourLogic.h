// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYParkourLogic.generated.h"

UENUM(BlueprintType)
enum class EParkourMontageType : uint8
{
	Stand_L, Stand_R,
	Walk_L, Walk_R,
	Run_L, Run_R,
	None
};

USTRUCT()
struct FGYTargetData_Parkour : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector TopHitLoc = FVector::ZeroVector;
	UPROPERTY()
	EParkourMontageType ParkourType = EParkourMontageType::None;

	virtual UScriptStruct* GetScriptStruct() const override{ return FGYTargetData_Parkour::StaticStruct(); }
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << TopHitLoc;
		uint8 MontageByte = static_cast<uint8>(ParkourType);
		Ar << MontageByte;

		if (Ar.IsLoading())
		{
			ParkourType = static_cast<EParkourMontageType>(MontageByte);
		}
		bOutSuccess = true;
		return true;
	}
};
template<>
struct TStructOpsTypeTraits<FGYTargetData_Parkour> : public TStructOpsTypeTraitsBase2<FGYTargetData_Parkour>
{
	enum
	{
		WithNetSerializer = true
	};
};

class UGYParkourFragment;
/**
 *
 */
UCLASS()
class GY_API UGYParkourLogic : public UAbilityLogicBase
{
	GENERATED_BODY()
public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual void OnInputPressed() override;
	virtual void OnInputReleased() override;

	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;


	void TryParkour();
	void ExecuteParkour(FVector& TopHitLoc, EParkourMontageType MontageType);
	EParkourMontageType SelectParkourMontage(bool bLeftFoot);

	bool DoForwardTrace(FHitResult& OutHit);
	bool DoTopTrace(FVector& WallLoc, FHitResult& OutHit);
	float GetMantleHeight(FVector& TopHitLoc); //장애물 높이
	UAnimMontage* SelectMontage(bool bLeftFoot);
	bool IsLeftFootForward(); // 어느발이 앞에있는지 판별
	void PlayMontage(UAnimMontage* Montage);

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnParkourDataRecive(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag tag);


private:
	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const UGYParkourFragment* CachedFragment = nullptr;
};
