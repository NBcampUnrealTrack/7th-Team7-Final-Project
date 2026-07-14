#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

// ECollisionChannel
constexpr ECollisionChannel ECC_HiddenWall		= ECC_GameTraceChannel1;
constexpr ECollisionChannel ECC_Traversable		= ECC_GameTraceChannel2;
constexpr ECollisionChannel ECC_Interactable	= ECC_GameTraceChannel3;
constexpr ECollisionChannel ECC_EnemyProjectile	= ECC_GameTraceChannel4;

// ETraceTypeQuery Alias — 전역 값으로 두면 정적 초기화에서 UCollisionProfile CDO 생성을 유발해
// 모놀리식(패키징) 빌드가 부팅 크래시. 반드시 호출 시점 변환 함수로 유지할 것
inline ETraceTypeQuery TraceType_HiddenWall() { return UEngineTypes::ConvertToTraceType(ECC_HiddenWall); }
inline ETraceTypeQuery TraceType_Traversable() { return UEngineTypes::ConvertToTraceType(ECC_Traversable); }
inline ETraceTypeQuery TraceType_Interactable() { return UEngineTypes::ConvertToTraceType(ECC_Interactable); }

// Collision Profile Name
// SetCollisionProfileName 호출 시 사용. 새 Preset 추가하면 여기에 같이 등록
namespace GYCollisionProfile
{
	inline const FName TraversableWall	= TEXT("TraversableWall");
	inline const FName MaskWall			= TEXT("MaskWall");
	inline const FName Interactable		= TEXT("Interactable");
}
