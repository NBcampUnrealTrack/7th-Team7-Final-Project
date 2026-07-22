<div align="center">

<br/>

# Grind Yesterday

**Unreal Engine 5 · C++ 기반 1~4인 코옵 라이트 소울류 탑다운 액션 RPG**

<br/>

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2F7UFlt%2FdJMcafOfhh4%2FAAAAAAAAAAAAAAAAAAAAADsxpAeEM4o57WAVXcvwUNOtivp7CdBvP5vF73GHqX-Z%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3DZsejyT1%252FnrsoZgd6cnGYgxd8nrA%253D" width="100%">

</div>

<br/>

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.7-0E1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows_PC-orange?style=for-the-badge)

![Netcode](https://img.shields.io/badge/Netcode-Dedicated_Server-5865F2?style=flat-square)
![Co-op](https://img.shields.io/badge/Co--op-1~4_Players-success?style=flat-square)
![GAS](https://img.shields.io/badge/Combat-Gameplay_Ability_System-informational?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-brightgreen?style=flat-square)

<br/>

🎬 [**트레일러**](https://www.youtube.com/watch?v=JlFGZAQV7JU) &nbsp;|&nbsp; 🎬 [**시연 영상**](https://www.youtube.com/watch?v=y3A0dVEEPEA) &nbsp;|&nbsp;
📄 [**최종 기획서**](https://app.notion.com/p/Grind-Yesterday-3a4711d71de1802b9501e3446787ab76?source=copy_link) &nbsp;|&nbsp; 📊 [**발표 자료**](https://www.canva.com/design/DAHP-AiEo2E/rSYgjcSeVh-G_0TWtLx3Iw/edit) &nbsp;|&nbsp;
<br>
📦 [**다운로드 링크**](https://dooyeonk.itch.io/gy/download/cPv093E5WfzdkrbeOiRQh8f9yYE4TzyalluTl3XS) &nbsp;|&nbsp; 💻 [**저장소**](https://github.com/NBcampUnrealTrack/7th-Team7-Final-Project)

<br/>

</div>

---

<div align="center">
  
<br/>

## 목차

<table>
<tr>
<td valign="top">

**개요**
- [1. 프로젝트 개요](#1-프로젝트-개요)
- [2. 게임 루프](#2-게임-루프)
- [3. 기술 스택](#3-기술-스택)

</td>
<td valign="top">

**게임플레이 시스템**
- [4. 조작 시스템](#4-조작-시스템)
- [5. 전투 시스템](#5-전투-시스템)
- [6. 카메라 · 타격감](#6-카메라--타격감)
- [7. 이동 · 상호작용](#7-이동--상호작용)
- [8. 몬스터 · 보스 AI](#8-몬스터--보스-ai)
- [9. 성장 · 스킬 트리](#9-성장--스킬-트리)
- [10. 인벤토리 · 장비](#10-인벤토리--장비--인챈트)
- [11. 시간의 틈](#11-시간의-틈)
- [12. 퀘스트](#12-퀘스트)
- [13. 볼륨 액터](#13-볼륨-액터)

</td>
<td valign="top">

**프레젠테이션 · 온라인**
- [14. 시네마틱](#14-시네마틱)
- [15. UI 시스템](#15-ui-시스템)
- [16. 사운드 시스템](#16-사운드-시스템)
- [17. 온라인 · 데디 서버](#17-온라인--데디케이트-서버)
- [18. 데이터 · 영속](#18-데이터--영속)

</td>
<td valign="top">
  
**아키텍처 · 툴링**
- [19. 에디터 툴링](#19-에디터-툴링)
- [20. 맵 구성](#20-맵-구성)
- [21. GameplayTag 구조](#21-주요-gameplaytag-구조)
- [22. 프로젝트 구조](#22-프로젝트-구조)
- [23. 코딩 컨벤션](#23-코딩-컨벤션)
- [24. 주요 설계 제약](#24-주요-설계-제약)

</td>
<td valign="top">
  
**기타**
- [25. 팀](#25-팀)
- [26. 라이선스](#26-라이선스)

</td>
</tr>
</table>

<br/>

</div>

---

<br/>

## 1. 프로젝트 개요

> 정밀한 근접 전투와 보스전을 중심에 둔 **1~4인 코옵 라이트 소울류 탑다운 액션 RPG.**
> *휴식–탐험–리셋* 사이클을 축으로, **호스트 없는 전용 서버 세션**과 상용 파이프라인을 지향해 개발했습니다.

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2Fd2efOM%2FdJMcafgm7hK%2FAAAAAAAAAAAAAAAAAAAAABvpP2d4gX6PgUWjso2E3S2EDWuRSfV6vII7F0txjNAx%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3DypVYRVWst6LxdCofq%252BHG9UMaGBc%253D" width="100%">

</div>

<br/>

| 항목 | 내용 |
|:---|:---|
| **제목** | Grind Yesterday · 프로젝트 코드네임 `GY` |
| **장르** | 라이트 소울류 **탑다운** 액션 RPG (1~4인 코옵) |
| **레퍼런스** | *No Rest for the Wicked* · *Elden Ring* |
| **엔진 / 언어** | Unreal Engine 5.7 · C++17 (Blueprint 병행) |
| **네트워크** | 데디케이트 서버 (호스트리스 세션) · 서버 권한 |
| **규모** | C++ 약 **920개** 파일 · 3개 모듈(`GY` / `GYUI` / `GYEditor`) |
| **팀 / 기간** | 8인 · 약 84일 |
| **목표** | Steam 출시용 버티컬 슬라이스 1종 = MVP |

<br/>

---

<br/>

## 2. 게임 루프

```text
마을(시간의 틈) ──▶ 구역 1 ──▶ 구역 보스 1 ──▶ 스토리 아이템 · 다음 구역 열쇠
                └▶ 구역 2 ──▶ 구역 보스 2 ──▶ 스토리 아이템 · 최종 관문 열쇠
                          └▶ 최종 보스 관문 ──▶ 최종 보스 ──▶ 엔딩
```

<br/>

| 사이클 | 내용 |
|:---|:---|
| **탐험 → 휴식** | '시간의 틈'에서 회복·저장하되, 대가로 **월드 시간 단축 → 몬스터 일괄 리젠** |
| **전리품 → 성장** | 장비를 제물로 바쳐 **시간의 파편** 획득 → 무기 가챠 · 인챈트 리롤 · 빌드 리스펙 |
| **설계 원칙** | *"코어 우선, 확장 후순위"* — 검+방패 / 1지역 / 보스 3종 버티컬 슬라이스, **카메라·타격감을 핵심 시스템으로 승격** |

<br/>

---

<br/>

## 3. 기술 스택

| 분류 | 기술 |
|:---|:---|
| **엔진 / 언어** | Unreal Engine 5.7 · C++17 · Blueprint |
| **전투 / 능력** | Gameplay Ability System · Gameplay Tags · Gameplay Tasks · Gameplay Cue · Motion Warping |
| **AI** | StateTree · Behavior Tree · EQS · Navigation |
| **모듈화** | GameFeatures · Modular Gameplay · Component Injection · Gameplay Message Router |
| **온라인** | OnlineSubsystem(Steam) · Dedicated Server · HTTP/JSON · Supabase (PostgreSQL · RLS · Edge Functions) |
| **UI / 입력** | CommonUI · CommonInput · Enhanced Input · UMG |
| **콘텐츠** | World Partition · PCG · Level Sequence · Niagara · DataAsset / DataTable / CurveTable |
| **에디터 툴링** | Slate · GraphEditor · UnrealEd · DataValidation |
| **인프라** | Git · Git LFS · GitHub Actions · AWS EC2 · itch.io · Jira · Poorforce(에셋 락) |

<br/>

---

<br/>

## 4. 조작 시스템

> **Enhanced Input** 을 Lyra 스타일로 래핑 — **입력을 GameplayTag로 추상화**해 하드코딩 없이 능력에 연결합니다.

<br/>

| 구성 | 역할 |
|:---|:---|
| `GYInputConfig` | `InputAction ↔ InputTag` 매핑 데이터 자산 |
| `GYInputComponent` | 태그 기준 입력 바인딩 (Pressed / Released / Triggered) |
| `GYPawnData` | 폰별 입력·능력 세트 데이터화 → GameFeature 로 런타임 부여 |

**입력 태그** &nbsp;`InputTag.Move · Attack · Charge · Parry · Dodge · Block · LockOn · Sprint · Parkour · Interact · Revive · UI.*`

> 입력 → `InputTag` → 능력 활성화가 전부 태그로 이어져, **리매핑·신규 액션 추가가 데이터 작업으로 끝납니다.**

<sub>`Source/GY/Character/{GYInputConfig, GYInputComponent, GYPawnData}` · `Core/GameplayTags/InputTag.h`</sub>

<br/>

---

<br/>

## 5. 전투 시스템

> **GAS 위에 얹은 데이터 주도 전투 아키텍처.** 능력을 상속이 아닌 **조합(composition)** 으로 구성해, 조합 폭발을 데이터 조립으로 해소합니다.

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2FoOw9m%2FdJMcaaTB8Ls%2FAAAAAAAAAAAAAAAAAAAAABm8dqBgmbctnK3JkSXkoMc2Kc2nzXzGWmq4UilzHVQJ%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3D9qtgDfZx14XtKHv2iAxKGc606h8%253D" width="100%">

</div>

<br/>

### ⚙️ *Fragment · Logic · Injector* 패턴

| 요소 | 타입 | 역할 |
|:---|:---|:---|
| `UAbilityFragment` | `UObject` | 차지 시간·슈퍼아머·몽타주·판정값 등 **데이터** 조각 (`FragmentTag` 식별) |
| `UAbilityLogicBase` | `UObject` | 패링/블록/회피/콤보/차지 **행동 규칙** 캡슐화 |
| `ULogicInjectorComponent` | `Component` | 런타임에 Fragment+Logic 을 능력에 **주입·조립** |
| `UAbilityFragmentModifierComponent` | `GameplayEffectComponent` | `GameplayEffect` 로 Fragment 프로퍼티 **런타임 수정** (버프가 차지 배율 +20% 등) |

<br/>

### 소울라이크 메커닉 — 독립 **누적 통(1:4)** 으로 "한 방의 무게" 구현

| 메커닉 | 발동 | 결과 |
|:---|:---|:---|
| **경직 내성 (HitRes)** | 통(100+STR) 소진 | Lv1 짧은 경직 · 콤보 캔슬 |
| **무력화 (Poise)** | 통(400) 소진 | Lv2 다운 · 약점 +50% |
| **패링 성공** | 적중 직전 ~0.2초 | 피해 0 + 적 무력화 풀 차감(즉시 Lv2) + 자기 통 풀 회복 |
| **방패 블로킹** | 우클릭 유지 | 1회에 **4채널 동시 변동** (플레이어 HP·무력화 / 적 반사 HP·무력화) |
| **슈퍼아머** | 보스/정예 차지 | 수치 무시 절대 무경직 |

**피격 4단계 우선순위** &nbsp;`슈퍼아머/잔여(0) > 경직(1) > 다운(2) > 강제 넉다운(3)`
**프레임 판정 노티파이** &nbsp;`AttackTrace · ComboWindow · TagAttachWindow · DelayCancel`

<details><summary><b>📐 핵심 산식 펼치기</b></summary>

<br/>

```cpp
HP 피해 = 무기 ATK × 모션 배율 × (1 + 1차 스탯 × 0.015) × 액션 보정
         × 치명타 배율 × (1 - DEF/(DEF+100)) × (블록 시 0.4)

경직/무력 차감 = 기본값 × 모션 배율 × 액션 보정
              // 블록: 경직 0 · 무력 -20 고정  /  패링: 자기 통 회복 · 적 무력 풀 차감

CritRate = 5% + DEX × 0.3%      CritDmg = 1.5배
```
</details>

<sub>`AbilitySystem/Abilities/{Fragment,Logic}` · `AttackLogic/{Parry,Block,Dodge,Combo,Charge,Notifies}` · `AbilitySystem/{Executions,Magnitudes,Attributes}`</sub>

<br/>

---

<br/>

## 6. 카메라 · 타격감

> 탑다운의 시각적 한계를 보완하기 위해 **카메라를 '전투 연출 도구'로 프레임워크화** — Mode · Effect · Volume 3분할.

<br/>

| 축 | 구현 | 설명 |
|:---|:---|:---|
| **상태 머신** | `GYCameraComponent` + `GYCameraModeData` | 데이터 구동 카메라 상태 관리 |
| **모드** | `CameraMode_Exploration` ↔ `Combat` | 전투 시 캐릭터–적 중점 오프셋 + 미세 줌인 |
| **이펙트** | `Shake` · `Zoom` · `Push` (`GYCameraEffectBase`) | **동시 3건↑이면 최강 1건만 적용(멀미 방지)** |
| **오클루전** | `CameraOcclusionComponent` | 장애물 반투명 + 외곽선 실루엣 |
| **록온** | `LockOnComponent` · `GA_LockOnToggle` | GAS 어빌리티 기반, 텔레그래프 화면 내 보정 |

**타격감 다층 피드백** &nbsp;채널별 히트스톱(0.05~0.2초) · 쉐이크 · 슬로우 · 플래시 · Niagara · 컨트롤러 진동을 데이터로 조합
**카메라 태그** &nbsp;`Camera.Mode.{Exploration, Combat, Boss, Boss.Phase2, Cinematic, ZoomIn/Out, Angle}`

<sub>`Source/GY/Camera/*` · `Character/{LockOn, CameraOcclusionMask}` · `Core/GameplayCue/*`</sub>

<br/>

---

<br/>

## 7. 이동 · 상호작용

> 트래버설과 월드 상호작용도 **GAS 어빌리티 + Motion Warping** 으로 통일.

<br/>

| 기능 | 구현 |
|:---|:---|
| **파쿠르 / 클라이밍** | `Ability.Parkour` · `Ability.Climb` · Motion Warping 으로 지형에 정합 |
| **사다리 / 문** | `Ability.Ladder.Activate` · `WorldGimmick/{Ladder, DoorActor, DoorMovementComponent}` |
| **상호작용** | GAS 기반 `Interaction/Abilities/Interactable` + `Interaction/Tasks` · `InputTag.Interact` |
| **부활** | `Character/Revive` · `InputTag.Revive` · `State.Life.BeingRevived` |

<sub>`Source/GY/Character/{Climbing, Revive}` · `AbilitySystem/Abilities/{Parkour, ParkourDodge}` · `Interaction/*` · `WorldGimmick/*`</sub>

<br/>

---

<br/>

## 8. 몬스터 · 보스 AI

> **StateTree · Behavior Tree · EQS 하이브리드.** 보스전을 소울류 클라이맥스로 설계.

<br/>

| 요소 | 구현 |
|:---|:---|
| **보스 StateTree AI** | `GYBossStateTreeAIComponent` — 페이즈 평가 · 패턴 선택/재생 · 어그로 · 경직/CC 반응 · 사망 상태 전이 |
| **평가자 / 태스크** | `PhaseEvaluator` · `AggroEvaluator` · `SelectPatternTask` · `StaggerReactTask` · `CCReactTask` |
| **EQS** | `EQC_TargetActor` · `EQC_OtherEnemies` (타깃·산개 위치 질의) |
| **BT 확장** | Decorator(`HasGameplayTag`, `CompareBB`) · Service(`SelectAbility`) · Task |
| **텔레그래프** | 지면 데칼/VFX/선딜 · 멀티는 **서버 트리거 → 클라 복제** |

**적 구성** &nbsp;일반 · 정예(방패병/광전사) · 구역 보스 ×2(페이즈 2단계) · 최종 보스

<sub>`Source/GY/Enemy/AI/{StateTree,EQS,Decorator,Services,Tasks}` · `Enemy/{Config,DataTables}`</sub>

<br/>

---

<br/>

## 9. 성장 · 스킬 트리

> **노드 트리 빌드 + 파밍 이원화.** 스킬 트리는 **커스텀 그래프 에디터로 저작**합니다 → [§19 에디터 툴링](#19-에디터-툴링)

<br/>

| 축 | 방법 | 결과 |
|:---|:---|:---|
| **빌드 (레벨업)** | 적 처치 → EXP → 노드 트리 투자 | 1차 스탯(STR/DEX) + 액션 노드 |
| **장비 (파밍)** | 필드 드랍 + 시간의 틈 가챠 | ATK/DEF/HP + 인챈트 |

| 항목 | 값 |
|:---|:---|
| **노드 트리** | 14P / **21노드 (7노드 강제 포기)** → 빌드 선택 강제 |
| **액션 노드** | 즉시 방어 · 빠른 반격 · 패링 카운터 (동시 **2개 활성화**) |
| **레벨 / EXP** | Lv1~15 · **CurveTable** 곡선 · 코옵 100% 균등 지급 |
| **리스펙** | '시간의 틈'에서 **무료** (데이터 자산 기반) |
| **Attribute** | `Vital`(HP/SP) · `CoreStat` · `Damage` · `Progression` (Player/Enemy 분리) |

<sub>`Source/GY/{SkillTree, Experience}` · `AbilitySystem/Attributes/*` · 저작 툴 → `Source/GYEditor/SkillTreeEditor/*`</sub>

<br/>

---

<br/>

## 10. 인벤토리 · 장비 · 인챈트

| 요소 | 설계 |
|:---|:---|
| **슬롯** | 2슬롯 — 무기(한손검+방패 세트) / 방어구 |
| **등급** | 노멀 · 스페셜 · 레전더리 · 각인 (인챈트 슬롯 0~3) |
| **드랍 스탯** | `베이스 × 등급 배율 × 레벨 배율 × (1 ± 0.05)` — 드랍마다 롤 |
| **인챈트** | 무기 7 / 방어구 7 / 페널티 5종 · **[최소~최대] 균등 분포 롤** · 툴팁 실제값 · 전부 `DataTable` |
| **아이템 구성** | **Fragment 조합**(`Items/Fragments`) · 분해(`Disassemble`)로 재화 환원 |
| **인벤토리 UI** | 리스트 슬롯 + 분류 탭(장비/소모품/퀘스트) + 드래그&드롭 |

<sub>`Source/GY/{Equipment, Inventory, Items, Enchant, Disassemble, Loot}` · 저작 툴 → `Source/GYEditor/ItemEditor/*`</sub>

<br/>

---

<br/>

## 11. 시간의 틈

> **회복 + 제물 바치기(가챠/리롤) + 숙련도 관리가 통합된 단일 거점** — 셋은 독립 UI로 개별 동시 상호작용.

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2FbLplqr%2FdJMcaaTB8Lr%2FAAAAAAAAAAAAAAAAAAAAAFW0eKSV1kvAsV8EeFitgdf6DPKACu0gSuFSiEpXkXfl%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3DZ7cG6LPULP0dahHq8MNzzP1Epw4%253D" width="100%">

</div>

<br/>

| 기능 | 비용 | 효과 / 대가 |
|:---|:---|:---|
| **휴식** | 월드 시간 −4h | HP/SP·물약 풀 회복 + 자동 저장 + 리스폰 갱신 → 몬스터 리젠 유도 |
| **무기 가챠** | 파편 50 | 랜덤 상위 장비 1점 |
| **인챈트 리롤** | 파편 15~ | 인챈트 슬롯 전체 재추첨 |
| **숙련도 리스펙** | 무료 | 노드 포인트 전체 환수·재분배 |

**구현** &nbsp;`TimeRift` + `TimeRiftSubsystem` + `TimeRiftData`(데이터 배치) · `CurrencyComponent`(시간의 파편 단일 통화, 플레이어 개별 소유)

<sub>`Source/GY/WorldGimmick/TimeRift/*` · `Currency/*` · `Core/GameplayTags/CurrencyTags.h`</sub>

<br/>

---

<br/>

## 12. 퀘스트

> 선형 사슬을 폐기하고 **투두 체크리스트** 방식으로 간소화.

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2FciFbRK%2FdJMcaaTB8Lq%2FAAAAAAAAAAAAAAAAAAAAAP9N49Rui6NtGM30ZviieRvf3Pr4xrMnh09N6vCf57MC%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3Dbob2Q1fhb2Vk%252BTZZPMa%252BVzv%252FA70%253D" width="100%">

</div>

<br/>

| 요소 | 구현 |
|:---|:---|
| **진행 관리** | `QuestSubsystem` — 자유 탐험 중 조건 완수 시 자동 체크 |
| **트리거** | `QuestTriggerVolume` — 지역·이벤트 진입 감지 |
| **잠금/해제** | 구역 보스 처치 → 스토리 아이템 · 다음 관문 열쇠 (자연 게이팅) |
| **태그** | `Quest.*` · UI 좌측 체크리스트 상시 표시 |

<sub>`Source/GY/Quest/{QuestSubsystem, QuestTriggerVolume, QuestTypes}` · `Core/GameplayTags/QuestTags.h`</sub>

<br/>

---

<br/>

## 13. 볼륨 액터

> 트리거 볼륨을 **공통 베이스에서 파생**해 지역·보스·퀘스트·카메라 연출을 데이터 배치로 처리.

<br/>

| 볼륨 | 용도 |
|:---|:---|
| `GYTriggerVolumeBase` | 공통 베이스 (오버랩 판정·필터) |
| `GYRegionVolume` | 지역 진입 감지 (월드 레벨 · BGM · 라이팅 컨텍스트) |
| `GYBossTriggerVolume` | 보스 아레나 진입 → 전투 개시·잠금 |
| `QuestTriggerVolume` | 퀘스트 조건 트리거 |
| `CameraVolumeActor` | 시점 강제 (Fixed / Angle / Zoom / Cinematic) · **멀티는 클라별 개별 적용** |

<sub>`Source/GY/World/VolumeActor/*` · `Camera/CameraVolumeActor.h` · `Quest/QuestTriggerVolume.h`</sub>

<br/>

---

<br/>

## 14. 시네마틱

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2FZR737%2FdJMcafgm7hN%2FAAAAAAAAAAAAAAAAAAAAAKTD2IaqgocgEvAFA6BxQL0SdmzDzJ-DFPRxXsp27z_V%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3Dtjz9V2DG9PVceE37c8oAObnNqCE%253D" width="100%">

</div>

| 요소 | 구현 |
|:---|:---|
| **연출** | **Level Sequence** — 보스 등장 인트로(`GYBossCharacterBase`/`GYChapterBossCharacter`) · 엔딩(`GYEndingInteractActor`) |
| **카메라 연동** | `Camera.Mode.Cinematic` + `CameraVolumeActor`(Cinematic) → 연출 후 표준 카메라 복귀 |
| **엔딩 UI** | `GYEndingCreditsWidget` · `GYEndingNarrativeWidget` |

<sub>`Source/GY/{Enemy/GYBossCharacterBase, WorldGimmick/GYEndingInteractActor}` · `GYUI/Widget/EndingCredits/*`</sub>

<br/>

---

<br/>

## 15. UI 시스템

> **CommonUI 기반 독립 모듈(`GYUI`)에 구축한 Lyra식 레이어드 UI 코어.**
> 게임플레이와 **직접 참조 0 - 전부 GameplayMessage(이벤트 버스)로 구동**해, UI가 게임 로직을 몰라도 동작하고 반대도 성립합니다.

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2FbIZ8ve%2FdJMcadvVYPa%2FAAAAAAAAAAAAAAAAAAAAAGsjvoUY_-Vn5wtjSmilpgpeXGQJXLSDGpM6ZqsTBBY7%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3Dr0iD4n9p45ROIkUVssOtJ4kf5xs%253D" width="100%">

</div>

<br/>

### 코어 아키텍처

| 컴포넌트 | 베이스 | 역할 |
|:---|:---|:---|
| `GYPrimaryGameLayout` | `CommonUserWidget` | **레이어 스택 컨테이너** — `UI.Layer.*` 태그별 스택에 Push/Pop, 씬 전환 시 ClearAll |
| `GYUIManagerSubsystem` | `LocalPlayerSubsystem` | **로컬 플레이어별 UI 총괄** — GameplayMessage 구독 → 위젯 오케스트레이션 |
| `GYActivatableWidget` | `CommonActivatableWidget` | 메뉴/모달 베이스 — Activate 시 **입력 모드(Game/Menu)·포커스 자동 전환** |
| `FGYTagDrivenWidgetEntry` | `struct` | **상태 태그 주도 표시** — `StateTag → LayerTag → WidgetClass`(+블로킹 여부) |
| `GYOwnerAwareWidgetComponent` | `WidgetComponent` | **소유자 인식 월드 위젯** — 멀티 플로팅 UI(네임플레이트/HP)의 오너 바인딩 |
| `GYConnectionStatusSubsystem` | `GameInstanceSubsystem` | **넷코드 UX** — 접속 끊김·트래블 실패 감지 → 안내 팝업 |
| `GYItemDragDropOperation` | `DragDropOperation` | 인벤토리 드래그&드롭 |
| `GYUISettings` | `DeveloperSettings` | UI 전역 설정 데이터화 |

<br/>

### 데이터 흐름 — *게임플레이 ↔ UI 완전 분리*

```text
게임플레이 이벤트 (보스 등장 · 지역 진입 · 부활 홀드 · 월드 리셋 · 엔딩 · 시계 오버레이 …)
        │  Broadcast (직접 참조 X)
        ▼
GameplayMessageRouter  ──▶  GYUIManagerSubsystem  ──▶  GYPrimaryGameLayout.PushWidgetToLayer(UI.Layer.*, Widget)
                             (메시지 구독·라우팅)         (레이어 스택이 입력·포커스·모달 자동 관리)
```

<br/>

### 위젯 카탈로그 (20+ 도메인)

| 그룹 | 위젯 |
|:---|:---|
| **HUD / 전투** | PlayerHUD · BossHUD · GaugeCircle · Floating(플로팅 데미지) · RegionInfo · Interact |
| **인벤토리 / 성장** | Inventory · Equipment · Slot · ItemInfo |
| **월드 / 코옵** | Loot · Revive · PlayerList · WorldReset(리젠 연출) · Character |
| **메뉴 / 시스템** | MainMenu · Settings · Common(팝업) · EndingCredits |

**설계 강점** &nbsp;① 메시지 버스로 결합도 제거 &nbsp;② 레이어 스택이 입력/모달 자동 처리 &nbsp;③ 상태 태그 주도 표시 &nbsp;④ 로컬 플레이어별 subsystem + owner-aware 위젯으로 **멀티플레이 정합** &nbsp;⑤ 넷코드 실패까지 UX 커버

<sub>`Source/GYUI/Core/*` (레이어·매니저·베이스) · `Widget/*` (도메인 위젯) · `Plugins/GameplayMessageRouter`</sub>

<br/>

---

<br/>

## 16. 사운드 시스템

> **태그·데이터 주도 사운드 파이프라인.**

<br/>

| 요소 | 구현 |
|:---|:---|
| **중앙 관리** | `GYSoundManager` — 재생 총괄, `SoundDataType` / `SoundSettings` 로 데이터화 |
| **AnimNotify** | `GYAN_Sound3D`(3D 위치 사운드) · `AnimNotify_FootStepSound`(표면별 발소리) |
| **태그** | `Sound.*` 로 이벤트 ↔ 에셋 느슨한 결합 |

<sub>`Source/GY/Core/Sound/*` · `Character/Animation/Notify/AnimNotify_FootStepSound.h` · `Core/GameplayTags/SoundTags.h`</sub>

<br/>

---

<br/>

## 17. 온라인 · 데디케이트 서버

> **호스트리스 데디케이트 세션 + 상용급 서버 오케스트레이션** — 이 프로젝트의 가장 큰 기술적 도전.

<br/>

| 계층 | 구현 |
|:---|:---|
| **접속** | OnlineSubsystem(Steam) + **Dedicated Server** 타깃 빌드 · 1~4인 드롭인 · 서버 권한 |
| **백엔드** | **Supabase** (PostgreSQL · RLS · Edge Functions) · 자체 `DataBridge` 플러그인이 UE↔REST 브릿지 |
| **인증** | `steam-auth` Edge Function → GoTrue 토큰 → 캐릭터 CRUD(RLS) |
| **세션 오케스트레이션** | `UGYWorldSessionSubsystem` — 목록/생성/삭제/입장 단일 진입점 · 하트비트 · `Requested→Queued→Starting→Online` 페이즈 통지 |
| **웜 스탠바이 풀** | `UWorldSessionComponent` — 대기 서버 `standby_servers` 등록·폴링 → 배정 시 세이브 로드 후 전환 (**콜드 스타트 제거**) |
| **인프라 자동화** | **AWS EC2** 자동 스폰/회수 · 슬롯(MaxWorlds) 관리 · stale 하트비트 리핑 |
| **권한 분리** | 클라 `anon`/Bearer(RLS) · 서버 `service_role`(RLS 우회, **커밋 금지·CI 주입**) |
| **코옵 규칙** | 협동 부활(HP −50%) · 인원수 스케일링(HP/무력화 1.0/1.4/1.7/2.0x) |

<sub>`Source/GY/WorldSession/*` · `{Account, Persistence}` · `Plugins/DataBridge` · `supabase/`</sub>

<br/>

---

<br/>

## 18. 데이터 · 영속

> **"밸런스는 코드 밖에" 원칙** — 수치·확률·연출 파라미터를 전부 자산으로 분리.

<br/>

| 축 | 구현 |
|:---|:---|
| **데이터 드리븐** | 스탯 / 드랍 / 인챈트(min·max) / 월드 스케일링 / 카메라 → `DataAsset` · `DataTable` · **`CurveTable`**(레벨·월드 8키포인트) |
| **영속 / 세이브** | `GYPersistenceSubsystem` — 안식·보스 처치 시 자동 저장 · **서버 권한** · `GYSaveable` 인터페이스 표준화 |
| **세이브 컴포넌트** | `WorldSaveComponent` · `CharacterSaveComponent` · `StatPersistenceComponent` |
| **원격 영속** | Supabase 에 계정·캐릭터·월드 세이브 저장 (service_role 서버 경로) |
| **디버그 콘솔** | `god` · `giveitem` · `setworldlevel` · `respec` · `addshards` · `cameramode` … |

<sub>`Source/GY/Persistence/*` · `Cheats/*` · `Enemy/DataTables/*`</sub>

<br/>

---

<br/>

## 19. 에디터 툴링

> **디자이너 생산성을 위한 자체 에디터 툴 — Slate·GraphEditor·DataValidation 위에 직접 구축한 툴 프로그래밍.**

<br/>

### 스킬 트리 그래프 에디터

블루프린트/비헤이비어 트리 에디터와 같은 방식의 **커스텀 노드-그래프 에디터**. 디자이너가 노드를 잇고 배치해 스킬 트리를 **시각적으로 저작**하면 `SkillTreeDataAsset` 으로 산출되어 런타임(`SkillTreeComponent`)이 소비합니다.

| 레이어 | 클래스 | 역할 |
|:---|:---|:---|
| **에디터 툴킷** | `SkillTreeAssetEditor` | 애셋 더블클릭 시 열리는 전용 에디터 |
| **그래프 모델** | `SkillTreeGraph` · `EdGraphNode_SkillNode` | `UEdGraph` 기반 노드/연결 데이터 |
| **연결 규칙** | `SkillTreeGraphSchema` · `SchemaAction` | 노드 생성·연결 유효성 규칙 |
| **Slate 비주얼** | `SGraphNode_SkillNode` · `SGraphPin_SkillPin` | 노드/핀 렌더링 위젯 |
| **팩토리 / 등록** | `SkillTreeNodeFactory` · `AssetTypeActions_SkillTree` | 노드 위젯 팩토리 · 애셋 타입 등록 |

<br/>

### 아이템 에디터 스위트

아이템 정의를 **저작·검증·풀 관리** 하는 원스톱 도킹 에디터.

| 구성 | 클래스 |
|:---|:---|
| **윈도우 / 컨트롤러** | `SGYItemEditorWindow` · `GYItemEditorController` · `GYItemEditorRegistration` |
| **패널 (5종)** | `Browser` · `Detail` · `Stats` · `Pool` · `Validation` |
| **생성 위저드** | `SGYItemCreationWizard` · `GYItemCreationLib` · `GYItemPresets` |
| **데이터 검증** | `GYEditorValidator_ItemDefinition`(DataValidation) + `Rules`/`Context`/`Types` — 규칙 위반 저장/커밋 차단 |
| **저장 락 연동** | `GYItemEditorSaveLock` — 저장 시 **Poorforce LFS 락** 자동 처리 |

<br/>

### 기타 툴

| 툴 | 설명 |
|:---|:---|
| `GYDebugMenu` / `GYDebugMenuManager` | 인게임/에디터 디버그 메뉴 |
| `UAMod_*`(Animation Modifier) | 발 위치 자동화 등 배치 애님 가공 |
| `LevelPlacedActorDataExporter` | 배치 액터 데이터 추출 (밸런스/분석용) |

<sub>`Source/GYEditor/{SkillTreeEditor, ItemEditor, Debug, Animation, EditorUtility}`</sub>

<br/>

---

<br/>

## 20. 맵 구성

> **World Partition 메인 맵 + Level Instance 서브레벨 + 기능별 테스트 맵**으로 모듈화.

<div align="center">
  <img src="https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdna%2FdjxUjQ%2FdJMcafgm7hL%2FAAAAAAAAAAAAAAAAAAAAAHnTENQDSrq2YakAXO5my5LlxOnF7HZYDgtJSeRnjcn9%2Fimg.png%3Fcredential%3DyqXZFxpELC7KVnFOS48ylbz2pIh7yKj8%26expires%3D1785509999%26allow_ip%3D%26allow_referer%3D%26signature%3DUFbxWAR%252BqU%252FgWHuPOF9kQylgVkI%253D" width="100%">

</div>

<br/>

| 맵 | 역할 |
|:---|:---|
| `L_Expanse_WP` | **메인 게임 월드** (World Partition) · ServerDefaultMap |
| `L_MainMenu_Temp` | 메인 메뉴 · GameDefaultMap |
| `LI_BossArea` · `LI_SampleArea3` · `LI_Login_Back` | **Level Instance** 서브레벨 (보스 아레나·구역·로그인 배경) |
| `LI_RuinedBridge` · `LI_RuinedRoad` … | 환경 프롭 인스턴스 (재사용 조립) |
| `L_EnemyTestMap_HitStop` · `L_CameraTestMap` · `L_PCGTestMap` … | **기능별 격리 테스트 베드** |

<sub>`Content/GY/Maps/{GameMaps, Instance, Test}` · 총 29 umap</sub>

<br/>

---

<br/>

## 21. 주요 GameplayTag 구조

> **Native GameplayTag** 를 도메인별 헤더로 분리(24개)해 컴파일 타임 안전성·자동완성 확보. 네임스페이스 `GYGameplayTags` / `GYStateTags`.

<br/>

```text
Ability.
├─ Attack.{Combo, Charge}   Guard  Parry  Block  Dodge  Sprint  Parkour  Climb  Drink
└─ Fragment.{Charge, Attack, Collision, Parry, Dodge, ComboMontage, ...}   // 데이터 조각 식별

InputTag.{Move, Attack, Charge, Parry, Dodge, Block, LockOn, Sprint, Interact, Revive, UI.*}

State.
├─ Life.{Alive, LowHP, Downed, BeingRevived, Dead}
├─ Hit.{Stagger, KnockDown, Stun}
└─ Combat.{InCombat, Attacking, SuperArmor, Invulnerable}   Cancelable  LockOn

Camera.Mode.{Exploration, Combat, Boss, Boss.Phase2, Cinematic, ...}
UI.Layer.{...}   // 레이어 스택 바인딩
```

**기타 도메인** &nbsp;`Effect · Cooldown · Penalty · Event · GameplayCue · Interaction · Equipment · Item · Enchant · Currency · Quest · Region · Faction · Window · Option · Anim · GrantSource · GameFeaturesInit`

<sub>`Source/GY/Core/GameplayTags/*` (24 헤더)</sub>

<br/>

---

<br/>

## 22. 프로젝트 구조

```
Grind Yesterday (GY)/
├── Source/
│   ├── GY/            # 게임플레이 런타임 (GAS 전투 · AI · 온라인 · 월드)
│   │   ├── AbilitySystem/  AttackLogic/  Camera/  Character/   # 전투 · 카메라 · 조작 · 트래버설
│   │   ├── Enemy/  WorldSession/  Account/  Persistence/       # AI · 네트워크 · 세이브
│   │   ├── SkillTree/ Experience/ Equipment/ Inventory/ Items/ Enchant/ Loot/  Currency/
│   │   ├── WorldGimmick/ Quest/ World/ Interaction/            # 시간의 틈 · 퀘스트 · 볼륨
│   │   └── GameFeatures/ Core/                                 # 모듈러 · 태그/사운드/타입
│   ├── GYUI/          # CommonUI 레이어드 UI 코어 (Core/ + Widget/ 20+ 도메인)
│   └── GYEditor/      # 커스텀 에디터 툴 (SkillTreeEditor · ItemEditor · Debug)
├── Plugins/          # DataBridge(서브모듈) · GameFeatures · GameplayMessageRouter · Poorforce
├── supabase/         # 백엔드 (마이그레이션 · steam-auth · 로컬 스택)
├── scripts/          # 패키징 · 에셋 동기화 · EC2 배포 오케스트레이션
├── .github/workflows/ # 클라/서버 패키징 · EC2 · 릴리즈 자동화
└── GY.uproject
```

<br/>

---

<br/>

## 23. 코딩 컨벤션

| 항목 | 규칙 |
|:---|:---|
| **네이밍** | `GY` 프로젝트 프리픽스 + UE 표준(`U`/`A`/`F`/`E`) · GameplayTag 네임스페이스(`GYGameplayTags`) |
| **모듈 경계** | 런타임 `GY` / UI `GYUI` / 에디터 `GYEditor` 분리 · 헤더 `Public/` · 구현 `Private/` |
| **포맷팅** (`.editorconfig`) | UTF-8 · **탭 들여쓰기(size 4)** · CRLF · 최종 개행 · 후행 공백 제거 |
| **소스 관리** | 소스 LF 저장/CRLF 체크아웃 · 바이너리(`*.uasset`/`*.umap`)는 **Git LFS + lockable**(Poorforce) |
| **데이터 우선** | 밸런스 수치는 코드가 아닌 `DataAsset`/`DataTable`/`CurveTable` |
| **협업** | Jira 이슈 연동 PR · `PULL_REQUEST_TEMPLATE.md` · `CODEOWNERS` |

<sub>`.editorconfig` · `.gitattributes` · `.github/*`</sub>

<br/>

---

<br/>

## 24. 주요 설계 제약

| 제약 | 내용 |
|:---|:---|
| **스코프** | *코어 우선 확장 후순위* — 검+방패 단일 / 1지역 / 보스 3종 버티컬 슬라이스 |
| **서버 권한** | 진행도·세이브·드랍·텔레그래프는 서버 트리거 → 클라 복제 (호스트리스) |
| **보안** | `service_role` 키 등 민감정보 커밋 금지 → CI Secret/ENV 주입 (Secret Scanning 대응) |
| **빌드** | 데디 **Server 타깃은 소스 빌드 엔진** 필수 (런처 엔진 불가) |
| **CI 함정** | 워크플로우 `run:` 블록은 **ASCII만** (PS 5.1 CP949 파싱 이슈 → 한글은 주석/name에만) |
| **에셋 협업** | 바이너리 동시 편집은 **LFS 락(Poorforce)** 으로 차단 · 유료 에셋은 외부 스토리지 분리 |

<br/>

---

<br/>

## 25. 팀

<br/>

**본인 주요 기여**

| GitHub | 담당 |
|:---|:---|
| [@hyun0619](https://github.com/hyun0619) | UIUX / 기획 |
| [@jcmeve](https://github.com/jcmeve) | 어빌리티 설계 / 코어 시스템 |
| [@saltlake00](https://github.com/saltlake00) | 레벨디자인 |
| [@yoonseo4343](https://github.com/yoonseo4343) | 네러티브 / TA |
| [@dooyeonk](https://github.com/dooyeonk) | 아이템 + 인벤토리 / 게임 인프라 |
| [@eunseoGithub](https://github.com/eunseoGithub) | 몬스터 AI |
| [@kbrother102](https://github.com/kbrother102) | 캐릭터 - 모션 / 스토리 |
| [@ood11611doo](https://github.com/ood11611doo) | 캐릭터 -전투 |


<br/>

---

<br/>

## 26. 라이선스

**MIT License**.

<br/>

<div align="center">

**Grind Yesterday · 내일배움캠프 Unreal 트랙 7기 · 7팀 최종 프로젝트**

본 프로젝트는 팀 내부 개발 프로젝트입니다.  
외부 공개 및 배포는 팀의 동의가 필요합니다.

---

*README 최종 업데이트: 2026.07*

<!-- ── 링크 정의부: 아래 # 를 실제 URL 로 교체하세요 ── -->
[link-demo]: #
[link-spec-core]: #
[link-spec-challenge]: #
[link-slides]: #
[link-pkg-server]: #
[link-pkg-client]: #
[link-repo]: https://github.com/NBcampUnrealTrack/7th-Team7-Final-Project
