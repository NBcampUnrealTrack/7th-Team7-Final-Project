-- ============================================================
-- GY 영속 백엔드 — Supabase (Postgres) 초기 스키마 v2
-- 적용: 레포 루트에서  supabase db push  (호스티드에 적용, 로컬 스택 불필요)
-- v2: 계정을 GoTrue(auth.users)와 통합 — accounts.id = auth.users.id (uuid).
--   Steam 로그인은 Edge Function steam-auth 가 auth 유저 생성 + 세션 발급.
--   클라(authenticated)는 auth.uid() RLS 로 자기 캐릭터만, 세이브 쓰기는 데디(service_role) 전용.
-- ============================================================

-- ── 계정 (Steam 신원, id = auth.users.id) ───────────────────
create table accounts (
  id            uuid primary key,              -- = auth.users.id (FK 제약은 관례대로 안 검)
  steam_id      text unique not null,
  persona_name  varchar(50),
  created_at    timestamptz default now(),
  last_login_at timestamptz default now()
);

-- ── 캐릭터 (계정당 여러 개, 전역). 게임데이터는 jsonb 덩어리 ──
create table characters (
  id           bigint generated always as identity primary key,
  account_id   uuid not null,                 -- 논리적 참조 (FK 제약 안 검)
  name         varchar(20) not null,
  level        int  not null default 1,
  xp           int  not null default 0,
  data         jsonb not null default '{}',   -- 인벤/장비/인첸트 blob (큰 월드데이터는 Storage/S3)
  save_version int  not null default 0,
  created_at   timestamptz default now(),
  updated_at   timestamptz default now(),
  deleted_at   timestamptz
);
create index characters_account_id_idx on characters (account_id);

-- ── RLS ─────────────────────────────────────────────────────
alter table accounts   enable row level security;
alter table characters enable row level security;

-- ── 테이블 권한: 직접 만든 테이블이라 GRANT 필요 (RLS 우회와 별개) ──
grant all on table accounts   to service_role;
grant all on table characters to service_role;

-- 클라(authenticated): 계정 조회 + 캐릭터 메타 CRUD.
-- insert/update 는 컬럼 단위로 좁힘 — level/xp/data/save_version 은 클라가 못 씀 (데디 권위).
-- 소프트 삭제는 직접 update 가 아니라 delete_character RPC 로 (부활 차단 + RETURNING-RLS 자기모순 회피)
grant select on table accounts to authenticated;
grant select on table characters to authenticated;
grant insert (account_id, name) on table characters to authenticated;
grant update (name) on table characters to authenticated;
grant usage, select on all sequences in schema public to authenticated;

create policy accounts_own on accounts
  for select using (id = auth.uid());

create policy characters_select on characters
  for select using (account_id = auth.uid() and deleted_at is null);

create policy characters_insert on characters
  for insert with check (account_id = auth.uid());

-- deleted_at is null 조건: 소프트 삭제된 캐릭터의 수정 차단 (개명 등 메타 갱신용)
create policy characters_update on characters
  for update using (account_id = auth.uid() and deleted_at is null)
  with check (account_id = auth.uid());

-- ── 소프트 삭제 = security definer RPC (직접 update 는 RETURNING 이 select 정책과 자기모순) ──
create or replace function delete_character(p_id bigint) returns boolean
language plpgsql security definer set search_path = public
as $$
begin
  update characters
     set deleted_at = now(), updated_at = now()
   where id = p_id and account_id = auth.uid() and deleted_at is null;
  return found;
end; $$;
revoke execute on function delete_character from public, anon;
grant execute on function delete_character to authenticated, service_role;

-- ── 저장 = 원자적 낙관적 동시성 (save_version 일치할 때만 갱신 +1) ──
-- 반환: 새 save_version / 충돌(버전 불일치)·없음이면 null
create or replace function save_character(
  p_id bigint, p_level int, p_xp int, p_data jsonb, p_expected_version int
) returns int
language plpgsql
as $$
declare v_new int;
begin
  update characters
     set level = p_level, xp = p_xp, data = p_data,
         save_version = save_version + 1, updated_at = now()
   where id = p_id and deleted_at is null and save_version = p_expected_version
   returning save_version into v_new;
  return v_new;   -- null = 충돌 또는 없음
end; $$;

-- 세이브 쓰기는 데디 권위 — 클라 실행 차단 (함수 기본 grant 가 public 이라 명시 revoke).
-- public revoke 는 service_role 의 경로도 끊으므로 명시 grant 필수 (RLS 우회는 테이블 얘기, 함수 권한은 별개)
revoke execute on function save_character from public, anon, authenticated;
grant execute on function save_character to service_role;

-- ── 캐릭터 수 제한 (클라 UX 체크의 서버측 백스톱) ──
create or replace function enforce_character_limit()
returns trigger language plpgsql as $$
declare v_count int;
begin
  select count(*) into v_count from characters
   where account_id = new.account_id and deleted_at is null;
  if v_count >= 3 then                  -- 정책값: 계정당 최대 캐릭터 수
    raise exception 'character limit reached';
  end if;
  return new;
end; $$;
create trigger trg_character_limit
  before insert on characters
  for each row execute function enforce_character_limit();

-- ── dev 테스트 시드: 캐릭터 id 1 (UE 콘솔 gy.Persist.Load/Save 1 용, service_role 경로라 auth 유저 불필요) ──
insert into accounts (id, steam_id, persona_name)
  values ('00000000-0000-0000-0000-000000000001', 'dev_seed', 'DevSeed');
insert into characters (account_id, name)
  values ('00000000-0000-0000-0000-000000000001', 'TestChar');
