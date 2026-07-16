-- 웜 스탠바이 풀 — 맵까지 부팅한 채 월드 배정을 기다리는 서버 목록.
-- 목적: 온디맨드 입장의 콜드 스타트(수십 초~분) 제거. 전부 service_role 전용 (클라 관여 없음).
-- 흐름: 서버 부팅 → register → 오케스트레이터가 시작 요청에 assign(원자) → 서버가 폴링으로 감지
--       → 세이브 로드 → worlds 하트비트(online) → consume(행 삭제). 죽은 스탠바이는 하트비트 리핑.

create table standby_servers (
  id                bigint generated always as identity primary key,
  host_addr         text not null,          -- ip:port
  assigned_world_id bigint,                 -- null = 대기 중
  heartbeat_at      timestamptz not null default now(),
  created_at        timestamptz not null default now()
);

alter table standby_servers enable row level security;
grant all on table standby_servers to service_role;

-- 스탠바이 서버 자기 등록
create or replace function register_standby(p_addr text) returns bigint
language plpgsql as $$
declare v_id bigint;
begin
  insert into standby_servers (host_addr) values (p_addr) returning id into v_id;
  return v_id;
end; $$;
revoke execute on function register_standby from public, anon, authenticated;
grant execute on function register_standby to service_role;

-- 스탠바이 생존 신고
create or replace function standby_heartbeat(p_id bigint) returns void
language plpgsql as $$
begin
  update standby_servers set heartbeat_at = now() where id = p_id;
end; $$;
revoke execute on function standby_heartbeat from public, anon, authenticated;
grant execute on function standby_heartbeat to service_role;

-- 원자 배정: 시작 요청된 offline 월드 하나를 가장 오래된 미배정 스탠바이에 붙인다.
-- 반환 = 배정된 standby id (null = 가용 스탠바이 없음 → 호출측이 콜드 스폰 폴백).
-- worlds 를 starting 으로 전환하므로 콜드 스폰 클레임과 이중 처리되지 않는다
create or replace function assign_standby_to_world(p_world_id bigint) returns bigint
language plpgsql as $$
declare v_standby_id bigint;
begin
  update worlds set status = 'starting'
   where id = p_world_id and deleted_at is null and status = 'offline'
     and start_requested_at is not null;
  if not found then return null; end if;

  select id into v_standby_id from standby_servers
   where assigned_world_id is null
   order by created_at
   limit 1
   for update skip locked;

  if v_standby_id is null then
    -- 스탠바이 없음 — 월드는 starting 으로 남고 호출측(오케스트레이터)이 콜드 스폰을 잇는다
    return null;
  end if;

  update standby_servers set assigned_world_id = p_world_id where id = v_standby_id;
  return v_standby_id;
end; $$;
revoke execute on function assign_standby_to_world from public, anon, authenticated;
grant execute on function assign_standby_to_world to service_role;

-- 배정 소화 완료 (서버가 월드 하트비트로 전환한 뒤 호출)
create or replace function consume_standby(p_id bigint) returns void
language plpgsql as $$
begin
  delete from standby_servers where id = p_id;
end; $$;
revoke execute on function consume_standby from public, anon, authenticated;
grant execute on function consume_standby to service_role;
