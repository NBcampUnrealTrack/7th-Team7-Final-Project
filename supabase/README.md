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

`Config\DefaultGYPersistence.ini` 는 **로컬 값(URL + demo 키)이 커밋돼 있어** 따로 생성 안 함.
(혹시 `supabase status` 의 service_role 키와 다르면 그 값으로 교체)

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

## 키 / 보안
- 로컬의 `ServiceRoleKey` = **로컬 demo 키(비밀 아님, 공개·고정)** → `Config\DefaultGYPersistence.ini` 에 커밋돼 있음
- **호스티드(실서버) service_role 키는 절대 커밋 금지** — 이 ini 에 넣지 말고 **CI/env 로 주입** (이 파일은 로컬 값으로 커밋됨)
- 클라 빌드엔 service_role 절대 X (RLS 우회 풀권한). 클라는 anon 키 사용

## 호스티드 (공유/배포 — 나중)
- supabase.com 프로젝트 생성 → `supabase\.bin\supabase.exe link --project-ref <ref>` → `db push`
- 호스티드 URL/키는 **커밋된 이 ini에 넣지 말 것** — 패키징 때 CI/env 로 주입 (클라=anon, 서버=service_role)
