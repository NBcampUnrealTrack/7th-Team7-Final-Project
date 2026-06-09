// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragmentModifierComponent.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragmentRegistry.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "AbilitySystem/Abilities/Logic/LogicInjectorComponent.h"
#include "Character/GYCharacter.h"
#include "Core/GameplayTags/EquipmentTags.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "AbilitySystem/Abilities/Logic/GYForceExecuteLogic.h"

UGYPlayerGameplayAbility::UGYPlayerGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bReplicateInputDirectly = true;
}


bool UGYPlayerGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (IsActive()) return false;

	if (const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (!GetLogic<UGYForceExecuteLogic>())
		{
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Handle == Handle) continue;
				if (Spec.IsActive() && Cast<UGYPlayerGameplayAbility>(Spec.Ability))
					return false;
			}

			if (const UGYPlayerAttribute* Attrs = ASC->GetSet<UGYPlayerAttribute>())
			{
				if (Attrs->GetCurrentStamina() <= 0.f)
					return false;
			}
		}
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGYPlayerGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (GetLogic<UGYForceExecuteLogic>())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			TArray<FGameplayAbilitySpecHandle> ToCancel;
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Handle == Handle || !Spec.IsActive()) continue;
				if (Cast<UGYPlayerGameplayAbility>(Spec.Ability))
					ToCancel.Add(Spec.Handle);
			}
			for (const FGameplayAbilitySpecHandle& CancelHandle : ToCancel)
			{
				if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CancelHandle))
					if (Spec->Ability) ASC->CancelAbility(Spec->Ability);
			}
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ScanAndApplyGEModifiers();

	for (UAbilityLogicBase* Logic : LogicList)
	{
		if (Logic)
		{
			Logic->OnExecute(this);
		}
	}
	for (UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (Logic)
		{
			Logic->OnExecute(this);
		}
	}

	if (!IsActive()) return;

	SetupEventListeners();
}

void UGYPlayerGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	for (UAbilityLogicBase* Logic : LogicList)
	{
		if (Logic)
		{
			Logic->OnAbilityEnd(this, bWasCancelled);
		}
	}
	for (UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (Logic)
		{
			Logic->OnAbilityEnd(this, bWasCancelled);
		}
	}
	InjectedLogics.Empty();
	RuntimeFragments.Empty();
	EventListenerTasks.Empty();
	CurrentDamageMultiplier = 1.f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGYPlayerGameplayAbility::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	for (UAbilityLogicBase* Logic : LogicList)
	{
		if (Logic) Logic->OnInputPressed();
	}
	for (UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (Logic) Logic->OnInputPressed();
	}
}

void UGYPlayerGameplayAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	for (UAbilityLogicBase* Logic : LogicList)
	{
		if (Logic) Logic->OnInputReleased();
	}
	for (UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (Logic) Logic->OnInputReleased();
	}
}



void UGYPlayerGameplayAbility::ScanAndApplyGEModifiers()
{
	//로직 리셋, Fragment build
	InjectedLogics.Empty();
	BuildRuntimeFragments();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayEffectQuery Query;
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
		if (!ActiveGE || !ActiveGE->Spec.Def) continue;

		const UGameplayEffect* GEDef = ActiveGE->Spec.Def;

		// Fragment Modifier
		if (const UAbilityFragmentModifierComponent* ModComp = GEDef->FindComponent<UAbilityFragmentModifierComponent>())
		{
			for (const FAbilityFragmentModifier& Mod : ModComp->Modifiers)
			{
				TObjectPtr<UAbilityFragment>* FragPtr = RuntimeFragments.Find(Mod.TargetFragmentTag);
				if (!FragPtr) continue;

				UAbilityFragment* Frag = FragPtr->Get();
				FFloatProperty* FloatProp = CastField<FFloatProperty>(
					Frag->GetClass()->FindPropertyByName(Mod.PropertyName));

				ensureMsgf(FloatProp != nullptr,
					TEXT("[%s] GE Modifier: Fragment '%s'에 프로퍼티 '%s' 없음."),
					*GetName(), *Mod.TargetFragmentTag.ToString(), *Mod.PropertyName.ToString());

				if (!FloatProp) continue;

				float Val = FloatProp->GetPropertyValue_InContainer(Frag);
				switch (Mod.ModOp.GetValue())
				{
				case EGameplayModOp::Additive:       Val += Mod.Magnitude; break;
				case EGameplayModOp::Multiplicitive: Val *= Mod.Magnitude; break;
				case EGameplayModOp::Override:       Val  = Mod.Magnitude; break;
				default: break;
				}
				FloatProp->SetPropertyValue_InContainer(Frag, Val);
			}
		}

		// LogicInject
		if (const ULogicInjectorComponent* InjComp = GEDef->FindComponent<ULogicInjectorComponent>())
		{
			// RequiredAbilityTags가 비어있으면 무조건 주입
			// 있으면 AbilityTags + 장착 무기 WeaponTags 합산 후 HasAll (AND) 검사
			bool bTagsMatch = InjComp->RequiredAbilityTags.IsEmpty();

			if (!bTagsMatch)
			{
				FGameplayTagContainer CombinedTags = GetAssetTags();

				// 장착 무기의 WeaponTags 추가
				if (const UEquipmentInstance* Weapon = GetCurrentWeapon())
				{
					const UItemFragment* ItemFragment =
						Weapon->GetItemDefinition()->FindFragmentByClass(UItemFragment_Weapon::StaticClass());
					if (ItemFragment)
					{
						CombinedTags.AddTag(static_cast<const UItemFragment_Weapon*>(ItemFragment)->WeaponTypeTag);
					}
				}

				bTagsMatch = CombinedTags.HasAll(InjComp->RequiredAbilityTags);
			}

			if (bTagsMatch)
			{
				for (const TObjectPtr<UAbilityLogicBase>& Template : InjComp->LogicsToInject)
				{
					if (Template) InjectedLogics.Add(DuplicateObject<UAbilityLogicBase>(Template, this));
				}
			}
		}
	}

	ValidateFragments();
}


void UGYPlayerGameplayAbility::BuildRuntimeFragments()
{
	RuntimeFragments.Empty();

	for (UAbilityFragment* Frag : Fragments)
	{
		if (!Frag) continue;

		ensureMsgf(
			!RuntimeFragments.Contains(Frag->FragmentTag),
			TEXT("[%s] BuildRuntimeFragments: 중복 FragmentTag '%s'. "),
			*GetName(), *Frag->FragmentTag.ToString());

		UAbilityFragment* RuntimeFrag = DuplicateObject<UAbilityFragment>(Frag, this);
		RuntimeFragments.Add(Frag->FragmentTag, RuntimeFrag);
	}
}


void UGYPlayerGameplayAbility::ValidateFragments() const
{
	for (const UAbilityLogicBase* Logic : LogicList)
	{
		if (!Logic) continue;
		for (const FGameplayTag& Required : Logic->GetRequiredFragmentTags())
		{
			ensureMsgf(
				RuntimeFragments.Contains(Required),
				TEXT("[%s] Logic '%s' requires Fragment '%s' but does not exist "),
				*GetName(),
				*Logic->GetClass()->GetName(),
				*Required.ToString());
		}
	}
	for (const UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (!Logic) continue;
		for (const FGameplayTag& Required : Logic->GetRequiredFragmentTags())
		{
			ensureMsgf(
				RuntimeFragments.Contains(Required),
				TEXT("[%s] Logic '%s' requires Fragment '%s' but does not exist "),
				*GetName(),
				*Logic->GetClass()->GetName(),
				*Required.ToString());
		}
	}
}

void UGYPlayerGameplayAbility::SetupEventListeners()
{
	TSet<FGameplayTag> UniqueTags;

	for (const UAbilityLogicBase* Logic : LogicList)
	{
		if (!Logic) continue;
		for (const FGameplayTag& Tag : Logic->GetSubscribedEventTags())
		{
			UniqueTags.Add(Tag);
		}
	}
	for (const UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (!Logic) continue;
		for (const FGameplayTag& Tag : Logic->GetSubscribedEventTags())
		{
			UniqueTags.Add(Tag);
		}
	}

	for (const FGameplayTag& Tag : UniqueTags)
	{
		UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, Tag, nullptr, false, true);
		Task->EventReceived.AddDynamic(this, &UGYPlayerGameplayAbility::OnGameplayEventDispatched);
		Task->ReadyForActivation();
		EventListenerTasks.Add(Task);
	}
}

void UGYPlayerGameplayAbility::OnGameplayEventDispatched(FGameplayEventData Payload)
{

	const FGameplayTag& EventTag = Payload.EventTag;

	for (UAbilityLogicBase* Logic : LogicList)
	{
		if (!Logic) continue;
		for (const FGameplayTag& Tag : Logic->GetSubscribedEventTags())
		{
			if (Tag == EventTag)
			{
				Logic->OnGameplayEvent(EventTag, Payload);
				break;
			}
		}
	}
	for (UAbilityLogicBase* Logic : InjectedLogics)
	{
		if (!Logic) continue;
		for (const FGameplayTag& Tag : Logic->GetSubscribedEventTags())
		{
			if (Tag == EventTag)
			{
				Logic->OnGameplayEvent(EventTag, Payload);
				break;
			}
		}
	}

}

float UGYPlayerGameplayAbility::PlayMontageForLogic(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage) return 0.f;

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, PlayRate, NAME_None, true);


	Task->ReadyForActivation();
	return Montage->GetPlayLength() / FMath::Max(PlayRate, KINDA_SMALL_NUMBER);
}



AGYCharacter* UGYPlayerGameplayAbility::GetGYCharacter() const
{
	return Cast<AGYCharacter>(GetAvatarActorFromActorInfo());
}

UEquipmentInstance* UGYPlayerGameplayAbility::GetCurrentWeapon() const
{
	const AGYCharacter* Character = GetGYCharacter();
	return Character ? Character->GetActiveEquipmentComponent()->GetEquippedInstance(GYGameplayTags::Equipment_Slot_Weapon) : nullptr;
}

#if WITH_EDITOR


void UGYPlayerGameplayAbility::SyncFragmentsToTags()
{
	if (!FragmentRegistry) return;

	// 기존 Fragment를 태그로 색인 (디자이너가 수정한 값 유지)
	TMap<FGameplayTag, UAbilityFragment*> Existing;
	for (UAbilityFragment* Frag : Fragments)
	{
		if (!Frag || !Frag->FragmentTag.IsValid()) continue;

		ensureMsgf(
			!Existing.Contains(Frag->FragmentTag),
			TEXT("[%s] SyncFragmentsToTags: 중복 FragmentTag '%s'. "
				 "같은 Fragment를 두 번 추가할 수 없습니다."),
			*GetName(), *Frag->FragmentTag.ToString());

		Existing.Add(Frag->FragmentTag, Frag);
	}

	// AbilityTags → 스키마 → 필요한 Fragment 목록 구성
	TArray<UAbilityFragment*> NewFragments;
	TSet<FGameplayTag> RequiredTags;

	for (const FGameplayTag& AbilityTag : GetAssetTags())
	{
		for (TSubclassOf<UAbilityFragment> FragClass : FragmentRegistry->GetRequiredFragmentClasses(AbilityTag))
		{
			if (!FragClass) continue;
			const UAbilityFragment* CDO = GetDefault<UAbilityFragment>(FragClass);
			if (!CDO || !CDO->FragmentTag.IsValid()) continue;

			if (RequiredTags.Contains(CDO->FragmentTag)) continue;
			RequiredTags.Add(CDO->FragmentTag);

			if (UAbilityFragment** Found = Existing.Find(CDO->FragmentTag))
			{
				NewFragments.Add(*Found); // 기존 값 유지
			}
			else
			{
				NewFragments.Add(NewObject<UAbilityFragment>(this, FragClass)); // 새로 생성
			}
		}
	}

	// 스키마에 없는 Fragment가 수동으로 남아있는지 경고
	for (const auto& Pair : Existing)
	{
		if (!RequiredTags.Contains(Pair.Key))
		{
			ensureMsgf(
				false,
				TEXT("[%s] SyncFragmentsToTags: Fragment '%s'가 현재 AbilityTags 스키마에서 요구하지 않습니다. "
					 "AbilityTags를 확인하거나 이 Fragment를 직접 제거하세요."),
				*GetName(), *Pair.Key.ToString());
		}
	}

	Fragments = NewFragments;
	Modify();

}

void UGYPlayerGameplayAbility::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SyncFragmentsToTags();
}

#endif
