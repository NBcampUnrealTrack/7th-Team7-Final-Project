#include "Persistence/CharacterSaveComponent.h"

#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "Account/GYAccountSubsystem.h"
#include "Currency/CurrencyComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Logging/GYLogManager.h"
#include "SkillTree/SkillTreeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	constexpr float RetryDelaySeconds = 5.f;

	// 델리게이트 없이 변하는 값(XP 등)을 쓸어담는 안전망. 유실 허용 창의 상한
	constexpr float PeriodicSaveSeconds = 180.f;
}

UCharacterSaveComponent::UCharacterSaveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UCharacterSaveComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority()) return;

	// 도메인 변경 감지 — 도메인 쪽은 저장을 모르게 유지 (구독은 여기서만)
	if (UCurrencyComponent* Currency = GetOwner()->FindComponentByClass<UCurrencyComponent>())
	{
		Currency->OnCurrencyChanged.AddWeakLambda(this, [this](FGameplayTag, int32) { RequestSave(); });
	}
	if (UInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		Inventory->OnInventoryChanged.AddWeakLambda(this, [this](const FGuid&, EInventoryEventType) { RequestSave(); });
	}
	if (UEquipmentLoadoutComponent* Loadout = GetOwner()->FindComponentByClass<UEquipmentLoadoutComponent>())
	{
		Loadout->OnLoadoutSlotChanged.AddWeakLambda(this, [this](FGameplayTag, FGuid) { RequestSave(); });
	}
	if (USkillTreeComponent* SkillTree = GetOwner()->FindComponentByClass<USkillTreeComponent>())
	{
		SkillTree->OnSkillTreeChanged.AddWeakLambda(this, [this]() { RequestSave(); });
	}

	// 레벨업 체크포인트. XP 는 구독하지 않음(전투 중 과도 발화)
	IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = Interface != nullptr ? Interface->GetAbilitySystemComponent() : nullptr;
	if (ASC != nullptr)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGYProgressionAttributeSet::GetLevelAttribute())
			.AddUObject(this, &UCharacterSaveComponent::OnLevelChanged);
	}

	// 주기 안전망 — XP 처럼 트리거 없는 변화 커버. dirty 조건 없이 무조건 저장(안 그러면 XP 에 대해 그물이 아님)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PeriodicTimerHandle,
			this,
			&UCharacterSaveComponent::OnPeriodicTimer,
			PeriodicSaveSeconds,
			true
		);
	}
}

void UCharacterSaveComponent::OnPeriodicTimer()
{
	if (!bLoaded) return;
	RequestSave();
}

void UCharacterSaveComponent::OnLevelChanged(const FOnAttributeChangeData& Data)
{
	RequestSave();
}

void UCharacterSaveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 flush (best-effort): dirty 여부와 무관하게 최종 상태 저장 —
	// XP 처럼 델리게이트 없이 변하는 값은 dirty 를 안 켜므로 무조건 전송이 맞다.
	// 엔진 종료 시엔 HttpManager 의 shutdown Flush 가 전송 완료를 시도한다.
	if (bLoaded && GetOwner()->HasAuthority())
	{
		GY_LOG(Network, KDY, "EndPlay flush (saving=%d)", bSaving);
		if (bSaving)
		{
			// in-flight 에 락이 잡혀 지금 못 보냄 + 컴포넌트는 곧 죽어 재시도 불가
			// → 수명이 긴 Subsystem 에 최종 스냅샷 인계, in-flight 완료 후 이어서 전송
			UGYPersistenceSubsystem* Persistence = ResolvePersistence();
			if (IsValid(Persistence))
			{
				int32 Level = 1;
				int32 Xp = 0;
				ReadLevelAndXp(Level, Xp);
				Persistence->HandoffLogoutSave(CharacterId, Level, Xp, Persistence->CollectSaveData(GetOwner()), CachedSaveVersion);
			}
		}
		else
		{
			bDirty = true;
			TrySave();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
		World->GetTimerManager().ClearTimer(PeriodicTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UCharacterSaveComponent::RequestSave()
{
	if (!GetOwner()->HasAuthority()) return;

	// 세이브 적용 중 Import가 발화시키는 도메인 델리게이트 무시 — 반쯤 적용된 스냅샷이 저장되는 것 방지
	if (bApplying) return;

	bDirty = true;

	// 즉시 전송하지 않고 다음 틱으로 — 같은 프레임의 연쇄 변경(획득 Added→Mutated 등)이
	// 끝난 완성 상태를 한 번에 스냅샷 (연산 중간 상태 저장 방지 + 프레임 내 뭉침)
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;
	World->GetTimerManager().SetTimerForNextTick(this, &UCharacterSaveComponent::TrySave);
}

void UCharacterSaveComponent::TrySave()
{
	// bLoaded 게이팅: version 동기화 전 저장은 무조건 충돌 → 로드 완료 후에만
	if (!bLoaded || bSaving || !bDirty) return;

	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return; // dirty 유지 — 다음 트리거에서 재시도

	bSaving = true;
	bDirty = false;

	const FString Data = Persistence->CollectSaveData(GetOwner());

	int32 Level = 1;
	int32 Xp = 0;
	ReadLevelAndXp(Level, Xp);

	Persistence->SaveCharacter(
		CharacterId,
		Level,
		Xp,
		Data,
		CachedSaveVersion,
		FGYOnSaveComplete::CreateUObject(this, &UCharacterSaveComponent::OnSaveDone)
	);
}

void UCharacterSaveComponent::OnSaveDone(const FGYSaveResult& Result)
{
	bSaving = false;

	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		CachedSaveVersion = Result.NewVersion;
		// 전송 중 들어온 변경을 한 번으로 뭉쳐 이어서 저장
		if (bDirty)
		{
			TrySave();
		}
		break;

	case EGYPersistResult::Conflict:
		// 버전 불일치 — 블라인드 재시도는 영구 충돌. 재로드로 version 재동기화 (로컬은 DB 상태로 덮임)
		GY_WARN(Network, KDY, "Save conflict - reloading to resync save_version");
		LoadAndApply();
		break;

	default:
		bDirty = true; // 실패한 스냅샷 변경분 유실 방지
		ScheduleRetry();
		break;
	}
}

void UCharacterSaveComponent::SetCharacterId(int64 InCharacterId)
{
	if (InCharacterId <= 0) return;

	if (bLoaded || bLoading)
	{
		GY_WARN(Network, KDY, "SetCharacterId(%lld) ignored - load already started (current=%d)", InCharacterId, CharacterId);
		return;
	}

	CharacterId = static_cast<int32>(InCharacterId);
	bCharacterIdExplicit = true;
}

void UCharacterSaveComponent::EnsureLoaded()
{
	if (bLoaded || bLoading) return;

	// listen/standalone 호스트: 접속 옵션이 없으므로 로컬 로그인 계정의 1:1 캐릭터 채택.
	// 로그인 미완료(레이스)/미로그인이면 dev stub 유지 — gy.Persist 콘솔 흐름 보존
	if (!bCharacterIdExplicit)
	{
		UWorld* World = GetWorld();
		UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;
		UGYAccountSubsystem* Account = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
		if (IsValid(Account) && Account->GetPrimaryCharacterId() > 0)
		{
			SetCharacterId(Account->GetPrimaryCharacterId());
		}
	}

	// 진짜 데디에 charId 없이 붙은 플레이어: stub(char 1)로 로드하면 남의/서로의 진행이 한 행에 섞인다
	// → 이 플레이어만 저장 비활성 (bLoaded false 유지 = 기존 게이팅이 저장 차단). PIE 데디는 stub 유지 (팀 반복 흐름)
	if (!bCharacterIdExplicit && IsValid(GetWorld())
		&& GetWorld()->GetNetMode() == NM_DedicatedServer && GetWorld()->WorldType != EWorldType::PIE)
	{
		GY_WARN(Network, KDY, "No charId for %s - persistence disabled (connect via gy.Account.Join)", *GetNameSafe(GetOwner()));
		return;
	}

	LoadAndApply();
}

void UCharacterSaveComponent::LoadAndApply()
{
	if (!GetOwner()->HasAuthority()) return;
	if (bLoading) return;

	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return;

	bLoading = true;
	Persistence->LoadCharacter(
		CharacterId,
		FGYOnLoadComplete::CreateUObject(this, &UCharacterSaveComponent::OnLoadDone)
	);
}

void UCharacterSaveComponent::OnLoadDone(const FGYLoadResult& Result)
{
	bLoading = false;

	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		{
			CachedSaveVersion = Result.SaveVersion;

			UGYPersistenceSubsystem* Persistence = ResolvePersistence();
			if (IsValid(Persistence) && Result.Data.IsValid())
			{
				bApplying = true;
				Persistence->ApplySaveData(GetOwner(), Result.Data);
				bApplying = false;
			}

			// 적용 직후 로컬 == DB. 적용 과정에서 도메인 델리게이트가 dirty 를 켜도 저장할 차이가 없다
			bDirty = false;
			bLoaded = true;
			GY_LOG(Network, KDY, "Character %d loaded (saveVersion=%d)", CharacterId, CachedSaveVersion);
			break;
		}

	case EGYPersistResult::NotFound:
		// 행 자체가 없으면 저장도 불가(RPC가 null 반환) — 게이팅 유지하고 경고만
		GY_WARN(Network, KDY, "Character %d not found - saving disabled", CharacterId);
		break;

	default:
		ScheduleRetry();
		break;
	}
}

void UCharacterSaveComponent::ScheduleRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	World->GetTimerManager().SetTimer(
		RetryTimerHandle,
		this,
		&UCharacterSaveComponent::OnRetryTimer,
		RetryDelaySeconds
	);
}

void UCharacterSaveComponent::OnRetryTimer()
{
	if (!bLoaded)
	{
		LoadAndApply();
	}
	else
	{
		TrySave();
	}
}

UGYPersistenceSubsystem* UCharacterSaveComponent::ResolvePersistence() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return nullptr;

	UGameInstance* GameInstance = World->GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYPersistenceSubsystem>() : nullptr;
}

void UCharacterSaveComponent::ReadLevelAndXp(int32& OutLevel, int32& OutXp) const
{
	OutLevel = 1;
	OutXp = 0;

	IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = Interface != nullptr ? Interface->GetAbilitySystemComponent() : nullptr;
	if (ASC == nullptr) return;

	OutLevel = static_cast<int32>(ASC->GetNumericAttributeBase(UGYProgressionAttributeSet::GetLevelAttribute()));
	OutXp = static_cast<int32>(ASC->GetNumericAttributeBase(UGYProgressionAttributeSet::GetXPAttribute()));
}
