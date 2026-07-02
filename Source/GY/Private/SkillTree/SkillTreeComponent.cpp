#include "SkillTree/SkillTreeComponent.h"

#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "Logging/GYLogManager.h"
#include "SkillTree/GYSkillTreeDataSettings.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTree/SkillTreeDataAsset.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Dom/JsonValue.h"
#include "Net/UnrealNetwork.h"

USkillTreeComponent::USkillTreeComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USkillTreeComponent, UnlockedNodes);
}

bool USkillTreeComponent::IsNodeUnlocked(const USkillNodeDataAsset* Node) const
{
	if (!Node) return false;
	return UnlockedNodes.Contains(Node->GetPrimaryAssetId());
}

bool USkillTreeComponent::HasAllPrerequisites(const USkillNodeDataAsset* Node) const
{
	if (!Node) return false;
	for (USkillNodeDataAsset* Prereq : Node->Prerequisites)
	{
		if (!Prereq) continue;
		if (!IsNodeUnlocked(Prereq)) return false;
	}
	return true;
}

bool USkillTreeComponent::UnlockNode(const USkillNodeDataAsset* Node)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Node) return false;
	if (IsNodeUnlocked(Node)) return false;

	UnlockedNodes.Add(Node->GetPrimaryAssetId());
	ApplySkillEffect(Node);
	OnSkillTreeChanged.Broadcast();
	return true;
}

void USkillTreeComponent::OnRep_UnlockedNodes()
{
	OnSkillTreeChanged.Broadcast();
}

TSharedPtr<FJsonValue> USkillTreeComponent::ExportSaveData() const
{
	TArray<TSharedPtr<FJsonValue>> Items;
	for (const FPrimaryAssetId& NodeId : UnlockedNodes)
	{
		Items.Add(MakeShared<FJsonValueString>(NodeId.ToString()));
	}
	return MakeShared<FJsonValueArray>(Items);
}

void USkillTreeComponent::ImportSaveData(const TSharedPtr<FJsonValue>& Data)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!Data.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!Data->TryGetArray(Items) || Items == nullptr) return;

	UAbilitySystemComponent* ASC = ResolveASC();

	// 재적용 멱등성: 이전에 적용한 노드 효과 제거 (충돌 재로드 등 반복 Import 대비)
	if (ASC != nullptr)
	{
		for (const FActiveGameplayEffectHandle& Handle : AppliedEffectHandles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}
	AppliedEffectHandles.Reset();

	UnlockedNodes.Reset();
	for (const TSharedPtr<FJsonValue>& Item : *Items)
	{
		FString IdString;
		if (!Item->TryGetString(IdString)) continue;

		const FPrimaryAssetId NodeId = FPrimaryAssetId::FromString(IdString);
		if (!NodeId.IsValid()) continue;

		UnlockedNodes.Add(NodeId);

		const USkillNodeDataAsset* Node = FindNodeById(NodeId);
		if (Node != nullptr)
		{
			ApplySkillEffect(Node);
		}
		else
		{
			// 트리에서 제거된 노드의 세이브 — 언락 목록엔 남기되 효과 없음
			GY_WARN(Network, KDY, "SkillTree import: node '%s' not found in tree", *IdString);
		}
	}

	if (ASC != nullptr)
	{
		// SP 잔량 = 스탯 복원이 세팅한 레벨 총량 − 사용분(노드당 1, GA_SkillUnlock 비용과 동일 전제)
		const float TotalSkillPoint = ASC->GetNumericAttributeBase(UGYProgressionAttributeSet::GetSkillPointAttribute());
		ASC->SetNumericAttributeBase(UGYProgressionAttributeSet::GetSkillPointAttribute(),
			FMath::Max(0.f, TotalSkillPoint - UnlockedNodes.Num()));

		// 노드 효과가 파생스탯(STR→MaxHealth 등)을 올린 뒤라 풀피 재충전 (스탯 복원의 풀피 정책 유지)
		ASC->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentHealthAttribute(),
			ASC->GetNumericAttribute(UGYVitalAttributeSet::GetMaxHealthAttribute()));
		ASC->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute(),
			ASC->GetNumericAttribute(UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute()));
	}

	OnSkillTreeChanged.Broadcast();
}

UAbilitySystemComponent* USkillTreeComponent::ResolveASC() const
{
	IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner());
	return Interface != nullptr ? Interface->GetAbilitySystemComponent() : nullptr;
}

const USkillNodeDataAsset* USkillTreeComponent::FindNodeById(const FPrimaryAssetId& NodeId) const
{
	const UGYSkillTreeDataSettings* Settings = GetDefault<UGYSkillTreeDataSettings>();
	USkillTreeDataAsset* Tree = Settings != nullptr ? Settings->SkillTreeDataAsset.LoadSynchronous() : nullptr;
	if (!IsValid(Tree)) return nullptr;

	TArray<const USkillNodeDataAsset*> Pending;
	TSet<const USkillNodeDataAsset*> Visited;
	for (USkillNodeDataAsset* Root : Tree->RootNodes)
	{
		if (Root != nullptr)
		{
			Pending.Add(Root);
		}
	}

	while (Pending.Num() > 0)
	{
		const USkillNodeDataAsset* Node = Pending.Pop();
		if (Visited.Contains(Node)) continue;
		Visited.Add(Node);

		if (Node->GetPrimaryAssetId() == NodeId) return Node;

		for (USkillNodeDataAsset* Child : Node->Children)
		{
			if (Child != nullptr)
			{
				Pending.Add(Child);
			}
		}
	}

	return nullptr;
}

void USkillTreeComponent::ApplySkillEffect(const USkillNodeDataAsset* Node)
{
	UAbilitySystemComponent* ASC = ResolveASC();
	if (ASC == nullptr || Node == nullptr || !Node->SkillEffect) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Node->SkillEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (Handle.IsValid())
	{
		// Instant GE 는 핸들이 invalid (base 영구 반영) — 추적 불필요. Duration/Infinite 만 제거 대상
		AppliedEffectHandles.Add(Handle);
	}
}
