#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Ladder.generated.h"

class UBoxComponent;

UCLASS()
class GY_API ALadder : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALadder();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	FORCEINLINE bool IsActivated() const { return bActivated; }
	FORCEINLINE bool CanClimb() const { return bCanClimb; }

	void Activate();

	FTransform GetClimbStartTransform(bool bFromTop) const;
	FVector GetClimbAxis() const;
	FRotator GetClimbFacing() const;

	float GetClimbDistance() const { return LadderHeight; }
	float GetRungSpacing() const;
	UBoxComponent* GetClimbCheckBox() const { return ClimbCheckBox; }

	UFUNCTION(BlueprintPure, Category="Ladder")
	UBoxComponent* GetBottomEntryBox() const { return BottomBox; }

	UFUNCTION(BlueprintPure, Category="Ladder")
	UBoxComponent* GetTopEntryBox() const { return ClimbIntoFromTopBox; }

protected:
	UFUNCTION()
	void OnRep_Activated();

	void BuildLadder();
	void UpdateColliders();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ladder")
	FDataTableRowHandle LadderDataHandle;

	//활성화 전 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ladder")
	float ActivateHeight = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ladder")
	float LadderHeight = 1000.f;

	UPROPERTY(EditAnywhere, Category="Ladder|Activation")
	float UnfoldDuration = 1.0f;

	//TODO 저장해야함 Save
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Activated)
	bool bActivated = false;

	UPROPERTY()
	bool bCanClimb = false;

	UPROPERTY(EditDefaultsOnly, Category="Ladder|Interaction")
	TSubclassOf<UGameplayAbility> ActivateAbilityClass;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Ladder|Visual")
	TObjectPtr<USceneComponent> LadderRoot;

	UPROPERTY(VisibleAnywhere, Category="Ladder|Collision")
	TObjectPtr<UBoxComponent> TopActivationBox;

	UPROPERTY(VisibleAnywhere, Category="Ladder|Collision")
	TObjectPtr<UBoxComponent> ClimbCheckBox;

	UPROPERTY(VisibleAnywhere, Category="Ladder|Collision")
	TObjectPtr<UBoxComponent> ClimbIntoFromTopBox;

	UPROPERTY(VisibleAnywhere, Category="Ladder|Collision")
	TObjectPtr<UBoxComponent> ClimbOutBox;

	UPROPERTY(VisibleAnywhere, Category="Ladder|Collision")
	TObjectPtr<UBoxComponent> BottomBox;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> RungMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> PoleMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> WallBracketMeshes;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TopGrabBarMesh;

private:
	//내리는거 관리
	float UnfoldAlpha = 0.f;
	bool  bUnfolding  = false;

};
