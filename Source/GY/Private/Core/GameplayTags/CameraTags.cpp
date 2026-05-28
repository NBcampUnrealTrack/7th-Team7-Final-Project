#include "Core/GameplayTags/CameraTags.h"

namespace GYGameplayTags
{
	// 카메라 모드
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_Exploration, "Camera.Mode.Exploration"); // 확대
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_Boss, "Camera.Mode.Boss"); // 보스 플레이어 위치 보간
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_Combat, "Camera.Mode.Combat"); // 전투
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_Cinematic, "Camera.Mode.Cinematic"); // 시네마틱
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_ZoomIn, "Camera.Mode.ZoomIn");
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_ZoomOut, "Camera.Mode.ZoomOut");
	UE_DEFINE_GAMEPLAY_TAG(Camera_Mode_Angle, "Camera.Mode.Angle");
}
