-- ============================================================
-- 월드 상태 영속 — worlds 테이블 + save_world/create_world RPC
-- 캐릭터(characters/save_character)와 같은 낙관적 동시성(CAS) 모델.
-- CAS 부수효과: 두 서버가 같은 월드를 동시에 호스팅하면 두 번째 writer 가
-- 즉시 Conflict 를 만나므로 싼 single-writer 보호가 된다.
-- 적용: supabase db push
-- ============================================================

create table worlds (
  id               bigint generated always as identity primary key,
  name             varchar(40) not null,
  owner_account_id uuid,                          -- 로비/roster 단계에서 사용 (지금은 null 허용)
  world_level      int  not null default 1,       -- 표시/검색용 denorm (권위 값은 data 안 섹션)
  data             jsonb not null default '{}',   -- 섹션 컨테이너: quest / world / reset ...
  save_version     int  not null default 0,
  created_at       timestamptz default now(),
  updated_at       timestamptz default now(),
  deleted_at       timestamptz
);

alter table worlds enable row level security;
grant all on table worlds to service_role;
-- 클라 접근(로비 월드 목록 등)은 필요해질 때 select 정책과 함께 추가

-- ── 저장 = save_character 와 동일한 CAS ──
create or replace function save_world(
  p_id bigint, p_world_level int, p_data jsonb, p_expected_version int
) returns int
language plpgsql
as $$
declare v_new int;
begin
  update worlds
     set world_level = p_world_level, data = p_data,
         save_version = save_version + 1, updated_at = now()
   where id = p_id and deleted_at is null and save_version = p_expected_version
   returning save_version into v_new;
  return v_new;   -- null = 충돌 또는 없음
end; $$;
revoke execute on function save_world from public, anon, authenticated;
grant execute on function save_world to service_role;

-- ── 신규 월드 보장 (로드 NotFound 분기에서 서버가 호출) ──
-- 명시 id insert 라 identity 시퀀스를 따라 올려줌 — 이후 자동 id 발급과 충돌 방지
create or replace function create_world(p_id bigint, p_name varchar)
returns int
language plpgsql
as $$
declare v_version int;
begin
  insert into worlds (id, name) overriding system value
  values (p_id, p_name)
  on conflict (id) do nothing;
  perform setval(pg_get_serial_sequence('worlds', 'id'),
                 greatest((select max(id) from worlds), 1));
  select save_version into v_version from worlds where id = p_id and deleted_at is null;
  return v_version;   -- null = 소프트 삭제된 월드 (호출측에서 Failure 취급)
end; $$;
revoke execute on function create_world from public, anon, authenticated;
grant execute on function create_world to service_role;

-- ── 팀 기본 월드 (서버 기본 -WorldId=1) ──
insert into worlds (id, name) overriding system value values (1, 'GY World 1');
select setval(pg_get_serial_sequence('worlds', 'id'), greatest((select max(id) from worlds), 1));
