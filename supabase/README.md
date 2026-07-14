# GY 백엔드 (로컬 Supabase) 셋업

UE 게임의 데이터 영속(세이브)을 처리하는 백엔드. 로컬에선 Supabase를 Docker로 띄워서 격리 테스트한다.
(공유/배포용 호스티드 Supabase는 별도 — 아래 "호스티드" 참고)

## 사전 준비 (수동, 1회)
- **Docker Desktop** 설치 → https://www.docker.com/products/docker-desktop/
  - 설치 시 **WSL2** 옵션, **재부팅 필요**. 설치 후 실행해서 트레이 고래 아이콘이 **녹색(실행중)** 인지 확인
  - (Docker만 수동 — 재부팅이 섞여 스크립트로 안 함)

## 셋업 (한 번)
`supabase\db-setup.bat` 더블클릭.
해당 배치파일 프로세스:
1. Docker 실행 확인 (없거나 안 켜졌으면 안내 후 종료)
2. **supabase CLI 바이너리 다운로드** (`supabase\.bin\` — Node/scoop/npm 불필요)
3. 첫 실행이면 `supabase init` (생성된 `config.toml`은 **커밋**하면 팀원은 init 생략)
4. `supabase start` + `supabase db reset` (마이그레이션 + 시드 적용)
5. `Config\DefaultGYPersistence.ini` 자동 생성 (로컬 supabase 의 URL/Secret key 주입)

> ini 는 **커밋하지 않는다**(로컬 키 포함, GitHub Secret Scanning 차단 회피). db-setup 이 각자 로컬 값으로 생성한다.
> 호스티드(실서버) 값은 패키징 시 CI/env 로 주입 (이 ini 에 넣지 말 것).

끝나면:
- Studio: http://127.0.0.1:54323 / API: http://127.0.0.1:54321

## 사용법 (supabase\ 폴더의 .bat 더블클릭)
| 파일 | 하는 일 |
|------|---------|
| `db-setup.bat` | 최초 셋업 (CLI 다운 + init + start + db reset) |
| `db-start.bat`  | 로컬 스택 켜기 |
| `db-stop.bat`   | 로컬 스택 끄기 (데이터 유지) |
| `db-reset.bat`  | 마이그레이션 + 시드 재적용 (스키마 갈아엎기)|
| `db-status.bat` | URL/키/상태 확인 |

- 스키마 바꿀 땐: `supabase\migrations\` SQL 수정 → `db-reset.bat`
- (초기 단계엔 마이그레이션 새로 안 쌓고 `init.sql` 직접 고쳐 reset 해도 됨)

## Steam 신원확인 (Edge Function `steam-auth`)
- 클라 로그인: UE `UGYAccountSubsystem` → `POST /functions/v1/steam-auth` → GoTrue 토큰 발급 → PostgREST 캐릭터 CRUD (RLS)
- 로컬은 **무설정으로 stub 모드** (steam_id 그대로 신뢰). 실검증은 자체 Steam App ID 확보 후 함수 secrets 로 전환:
  `STEAM_AUTH_MODE=verify` + `STEAM_WEB_API_KEY` + `STEAM_APP_ID`
- **자동 로그인**: 게임/PIE 시작 시 자동 인증 + 계정당 캐릭터 1개 자동 확보(1:1, persona 이름 — 생성/선택 UI는 추후). 서버 접속은 `gy.Account.Join <ip[:port]>` — 내 캐릭터 id가 접속 옵션으로 붙어 데디가 그 캐릭터로 저장/로드
- UE 콘솔: `gy.Account.Login [Mock|Steam]`(수동 재시도/모드 전환) / `gy.Account.Chars` / `gy.Account.CreateChar <name>` / `gy.Account.DeleteChar <id>` / `gy.Account.Join <ip[:port]>` / `gy.Account.SmokeTest`(로그인→생성→목록→삭제 왕복, 성공 시 "SmokeTest PASSED" 로그)
- Steam 모드는 Steam 클라 실행 + 엔진 `Binaries\Win64\steam_appid.txt`(내용 `480`) 필요. 에디터/PIE에서도 동작하나 에디터발 인스턴스는 전부 같은 SteamID — 다계정 테스트는 Mock(PIE 인스턴스별 `dev_test_<n>` 자동 분리)

## 키 / 보안
- 로컬의 `ServiceRoleKey` = **로컬 demo 키(비밀 아님, 공개·고정)** → `Config\DefaultGYPersistence.ini` 에 커밋돼 있음
- **호스티드(실서버) service_role 키는 절대 커밋 금지** — 이 ini 에 넣지 말고 **CI/env 로 주입** (이 파일은 로컬 값으로 커밋됨)
- 클라 빌드엔 service_role 절대 X (RLS 우회 풀권한). 클라는 anon 키 사용

## 호스티드 (공유/배포 — 나중)
- supabase.com 프로젝트 생성 → `supabase\.bin\supabase.exe link --project-ref <ref>` → `db push`
- 호스티드 URL/키는 **커밋된 이 ini에 넣지 말 것** — 패키징 때 CI/env 로 주입 (클라=anon, 서버=service_role)
