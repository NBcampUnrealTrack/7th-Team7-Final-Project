#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AreaWarningActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class GY_API AAreaWarningActor : public AActor
{
	GENERATED_BODY()

public:
	AAreaWarningActor();

	UFUNCTION(BlueprintCallable, Category = "AreaWarning")
	void Initialize(float InGrowDuration, float InWorldRadius);

	/** 폭발 없이 즉시 제거 (예: 페이즈 성공 시). */
	UFUNCTION(BlueprintCallable, Category = "AreaWarning")
	void Cancel();

	float GetWarningRadius() const { return WorldRadius; }

	/** ArenaCenterTag 를 가진 레벨 액터 위치를 반환. 없으면 Fallback 반환. */
	static FVector ResolveArenaCenter(UWorld* World, FName Tag, const FVector& Fallback);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "AreaWarning")
	TObjectPtr<UStaticMeshComponent> WarningMesh;

	/** 머티리얼에서 0→1 로 올릴 스칼라 파라미터 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "AreaWarning")
	FName GrowProgressParam = TEXT("GrowProgress");

	/** 0→1 까지 커지는 데 걸리는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "AreaWarning", meta = (ClampMin = "0.01"))
	float GrowDuration = 30.f;

	/** 성장 완료 후 액터를 자동 파괴할지 */
	UPROPERTY(EditDefaultsOnly, Category = "AreaWarning")
	bool bDestroyOnComplete = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WarningMID;

	float WorldRadius = 0.f;
	float Elapsed = 0.f;
	bool bGrowing = false;
};
