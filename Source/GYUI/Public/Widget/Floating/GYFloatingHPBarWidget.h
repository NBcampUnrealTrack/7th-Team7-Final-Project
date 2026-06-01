#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYFloatingHPBarWidget.generated.h"

class UProgressBar;
class UCommonTextBlock;
class UAbilitySystemComponent;
class APlayerState;
struct FOnAttributeChangeData;
struct FGYPlayerNameMessage;
struct FGYCharacterReadyMessage;

/**
 * World 기준으로 캐릭터 머리 위에서 표시되는 체력바
 * owner 종류에 따라 동작이 다름 - 로컬 플레이어는 안뜨게, 다른 플레이어는 +닉네임해서 항상 뜨게, 적은 변동 시만 뜨게
 */
UCLASS()
class GYUI_API UGYFloatingHPBarWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	UGYFloatingHPBarWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> NameText;

	/** AI 모드에서 마지막 변동 후 유지 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	float HoldDuration = 2.0f;

	/** AI 모드에서 페이드 아웃 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	float FadeSpeed = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	float BindGracePeriod = 0.5f;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|UI")
	void OnHealthUpdated(float Current, float Max);

private:
	enum class EBarMode : uint8
	{
		Hidden,
		PlayerAlways,
		EnemyFade
	};

	void BindToASC(UAbilitySystemComponent* InASC);
	void RefreshHealth();
	void HandlePlayerNameMessage(FGameplayTag Channel, const FGYPlayerNameMessage& Message);

	void TryBindToOwner(AActor* InCharacter);
	/** 메세지 수신 시 호출 */
	void HandleCharacterReadyMessage(FGameplayTag Channel, const FGYCharacterReadyMessage& Message);
	/** 소유하고 있는 액터 찾아오는 함수 */
	AActor* GetOwningActor() const;
	void SetWidgetOwnerActor(AActor* InOwner);

	EBarMode Mode = EBarMode::Hidden;

	TWeakObjectPtr<AActor> StoredOwner;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	TWeakObjectPtr<APlayerState> TargetPS;

	float  CurrentAlpha = 0.f;
	double LastActivityTime = -1000.0;
	double BindTime = -1000.0;

	/** 바인딩 재시도 */
	float BindRetryInterval = 0.25f;
	float BindRetryTimeout  = 5.f;

	bool  bTryingToBind = true;
	float BindRetryAccum = 0.f;
	float BindRetryElapsed = 0.f;
};
