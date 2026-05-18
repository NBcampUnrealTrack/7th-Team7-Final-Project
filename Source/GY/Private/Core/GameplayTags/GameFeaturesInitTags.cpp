// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/GameplayTags/GameFeaturesInitTags.h"

namespace GYGameplayTags
{

UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned",
                               "1:액터나 컴포넌트가 최초로 스폰, 기능추가가 가능한 상태 ");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable, "InitState.DataAvailable",
                               "2: 필요한 모든 데이터가 로드 되었거나 리플리케이션 되었으며, 초기화 준비가 된 상태");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized, "InitState.DataInitialized",
                               "3: 이용 가능한 데이터가 이 액터/컴포넌트에 적용(초기화)되었으나, 게임 플레이 준비는 되지 않음")
;
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady, "InitState.GameplayReady",
                               "4: 모든 준비가 끝낫당.");
}
