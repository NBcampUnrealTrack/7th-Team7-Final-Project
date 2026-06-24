-- ============================================================
-- GY 영속 백엔드 — Supabase (Postgres) 초기 스키마
-- 적용: 레포 루트에서  supabase db push  (호스티드에 적용, 로컬 스택 불필요)
-- 1단계: 데디(service_role)가 저장/불러오기까지.
--   Steam auth Edge Function + 클라용 RLS 정책 + char create = 다음 단계
-- ============================================================

-- ── 계정 (Steam 신원) ───────────────────────────────────────
create table accounts (
  id            bigint generated always as identity primary key,
  steam_id      text unique not null,
  persona_name  varchar(50),
  created_at    timestamptz default now(),
  last_login_at timestamptz default now()
);

-- ── 캐릭터 (계정당 여러 개, 전역). 게임데이터는 jsonb 덩어리 ──
create table characters (
  id           bigint generated always as identity primary key,
  account_id   bigint not null,               -- 논리적 참조 (FK 제약 안 검)
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

-- ── RLS: 정책 없이 켜두면 anon/authenticated 차단, service_role(서버)만 접근 ──
-- 클라이언트용 정책은 다음 단계(auth)에서. 지금은 데디(service_role)만 접근.
alter table accounts   enable row level security;
alter table characters enable row level security;

-- ── 테이블 권한: 직접 만든 테이블이라 service_role 에 GRANT 필요 ──
-- (RLS 는 service_role 을 우회하지만, 그 전에 테이블 권한이 먼저 있어야 함)
grant all on table accounts   to service_role;
grant all on table characters to service_role;

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

-- ── 캐릭터 수 제한 ──
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

-- ── dev 테스트 시드: 캐릭터 id 1 생성 (UE 콘솔 gy.Persist.Load/Save 1 용) ──
insert into accounts (steam_id, persona_name) values ('dev_test_1', 'DevTester');
insert into characters (account_id, name) values (1, 'TestChar');
