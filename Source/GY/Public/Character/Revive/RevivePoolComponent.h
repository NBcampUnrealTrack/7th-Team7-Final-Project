#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionOption.h"
#include "RevivePoolComponent.generated.h"

class AGYCharacter;
class AGYDownedDecorationActor;
class UGYReviveConfig;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRevivePoolPercentChanged, float, NewPercent);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API URevivePoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URevivePoolComponent();

	void ActivatePool(UGYReviveConfig* Config, const FVector& DeathLocation);
	void DeactivatePool();

	void StartReviving(AGYCharacter* Reviver);
	void StopReviving();

	UFUNCTION(BlueprintPure)
	float GetAccumulatedPercent() const { return AccumulatedPercent; }

	UFUNCTION(BlueprintPure)
	bool IsPoolActive() const { return bIsPoolActive; }

	UFUNCTION(BlueprintPure)
	bool IsBeingRevived() const { return CurrentReviver.IsValid(); }

	UFUNCTION(BlueprintPure)
	float GetRequiredPercent() const;

	void AppendInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const;
	void HandleInteract(FGameplayTag OptionTag, APawn* Interactor);

	UPROPERTY(BlueprintAssignable)
	FOnRevivePoolPercentChanged OnPoolPercentChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_AccumulatedPercent)
	float AccumulatedPercent = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_bIsPoolActive)
	bool bIsPoolActive = false;

	UFUNCTION()
	void OnRep_bIsPoolActive();

	UPROPERTY(Replicated)
	TObjectPtr<UGYReviveConfig> ActiveConfig;

	TWeakObjectPtr<AGYCharacter> CurrentReviver;
	TWeakObjectPtr<AGYDownedDecorationActor> DecorationActor;

	void CompleteRevive();

	/** 부활 진행도를 UI 메시지 버스로 방송 */
	void BroadcastProgress() const;

	UFUNCTION()
	void OnRep_AccumulatedPercent();
};
