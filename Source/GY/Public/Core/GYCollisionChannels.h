#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

// ECollisionChannel
constexpr ECollisionChannel ECC_HiddenWall		= ECC_GameTraceChannel1;
constexpr ECollisionChannel ECC_Traversable		= ECC_GameTraceChannel2;
constexpr ECollisionChannel ECC_Interactable	= ECC_GameTraceChannel3;
constexpr ECollisionChannel ECC_EnemyProjectile	= ECC_GameTraceChannel4;

// ETraceTypeQuery Alias
inline const ETraceTypeQuery TraceType_HiddenWall	= UEngineTypes::ConvertToTraceType(ECC_HiddenWall);
inline const ETraceTypeQuery TraceType_Traversable	= UEngineTypes::ConvertToTraceType(ECC_Traversable);
inline const ETraceTypeQuery TraceType_Interactable	= UEngineTypes::ConvertToTraceType(ECC_Interactable);

// Collision Profile Name
// SetCollisionProfileName 호출 시 사용. 새 Preset 추가하면 여기에 같이 등록
namespace GYCollisionProfile
{
	inline const FName TraversableWall	= TEXT("TraversableWall");
	inline const FName MaskWall			= TEXT("MaskWall");
	inline const FName Interactable		= TEXT("Interactable");
}
