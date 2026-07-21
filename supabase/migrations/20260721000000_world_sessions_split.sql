-- ============================================================
-- 월드 세션 재구성 — 런타임 상태를 worlds 에서 world_sessions 로 분리.
--
-- 원칙: "세션 행이 존재 = 활성, 행이 없음 = offline".
--   오프라인을 플래그가 아니라 '행 부재'로 표현해, 예전 구조의 유령 상태
--   (start_requested_at 는 남았는데 아무도 안 기다림)를 원천 차단한다.
--
-- 생존 판정 = 단계별 책임자의 마지막 신호:
--   requested 단계 → 대기 클라가 last_waiting_at 를 갱신(임대 갱신). 멈추면 만료 → 행 삭제.
--   online    단계 → 서버가 last_heartbeat_at 를 갱신(맥박). 멎으면 죽음 → 행 삭제.
--   starting  단계 → 오케스트레이터가 자기 프로세스로 추적(부팅 유예). DB reap 대상 아님.
--
-- FIFO: requested_at(최초 요청 시각, 고정) 오름차순 = 오래 기다린 사람 먼저.
-- 적용: supabase db push
-- ============================================================

-- ── worlds 에서 런타임 컬럼 제거 (영속 데이터만 남긴다) ──
alter table worlds
  drop column if exists host_addr,
  drop column if exists player_count,
  drop column if exists max_players,
  drop column if exists status,
  drop column if exists heartbeat_at,
  drop column if exists start_requested_at;

-- ── 런타임 세션 (worlds 와 1:1, 행 존재 = 활성) ──
create table world_sessions (
  world_id          bigint primary key references worlds(id) on delete cascade,
  status            text not null default 'requested',   -- requested | starting | online
  host_addr         text,                                 -- "ip:port" (online 일 때만 유효)
  player_count      int  not null default 0,
  max_players       int  not null default 4,
  requested_at      timestamptz not null default now(),   -- FIFO 정렬키 (최초 요청, 고정)
  last_waiting_at   timestamptz not null default now(),   -- 대기 클라 임대 갱신 (requested 만료 판정)
  last_heartbeat_at timestamptz                           -- 서버 맥박 (online 생존 판정)
);
create index world_sessions_status_idx on world_sessions (status);

alter table world_sessions enable row level security;
grant all on table world_sessions to service_role;
-- 클라는 목록 표시를 위해 세션 상태를 읽을 수 있어야 한다 (공개 정보)
grant select on table world_sessions to authenticated;
create policy world_sessions_select on world_sessions for select using (true);

-- ── 클라 목록용 플랫 뷰 (worlds + 세션 좌조인, 세션 없으면 offline) ──
-- security_invoker: 호출자(authenticated) 권한으로 실행 → worlds/world_sessions RLS 그대로 적용
create view world_list with (security_invoker = on) as
select
  w.id, w.name, w.world_level, w.owner_account_id, w.owner_name,
  coalesce(s.status, 'offline') as status,
  s.host_addr,
  coalesce(s.player_count, 0) as player_count,
  coalesce(s.max_players, 4)  as max_players
from worlds w
left join world_sessions s on s.world_id = w.id
where w.deleted_at is null;
grant select on world_list to authenticated;

-- ── 클라: 시작 요청 + 임대 갱신 (idempotent) ──
-- 세션 없으면 requested 로 생성, 이미 requested 면 last_waiting_at 만 갱신,
-- starting/online 이면 건드리지 않는다. 반환 = 처리 후 현재 status (offline=월드 없음).
-- 반환 타입이 boolean → text 로 바뀌어 CREATE OR REPLACE 불가 — 먼저 DROP
drop function if exists request_world_start(bigint);
create or replace function request_world_start(p_id bigint) returns text
language plpgsql security definer set search_path = public
as $$
declare v_status text;
begin
  if not exists (select 1 from worlds where id = p_id and deleted_at is null) then
    return 'offline';
  end if;

  insert into world_sessions (world_id, status, requested_at, last_waiting_at)
  values (p_id, 'requested', now(), now())
  on conflict (world_id) do update
     set last_waiting_at = now()
   where world_sessions.status = 'requested'
  returning status into v_status;

  -- starting/online 이라 위 update 가 건너뛰어졌으면 현재 상태를 읽어 반환
  if v_status is null then
    select status into v_status from world_sessions where world_id = p_id;
  end if;
  return coalesce(v_status, 'offline');
end; $$;
revoke execute on function request_world_start from public, anon;
grant execute on function request_world_start to authenticated, service_role;

-- ── 오케스트레이터: 스폰 클레임 (requested → starting 원자 전환) ──
-- 반환 true = 이 호출자가 스폰 권리 획득. 경합해도 한쪽만 true
create or replace function claim_world_start(p_id bigint) returns boolean
language plpgsql as $$
begin
  update world_sessions set status = 'starting'
   where world_id = p_id and status = 'requested';
  return found;
end; $$;
revoke execute on function claim_world_start from public, anon, authenticated;
grant execute on function claim_world_start to service_role;

-- ── 서버: 하트비트 (등록 겸용 — 첫 하트비트가 online 전환/행 생성) ──
create or replace function heartbeat_world(p_id bigint, p_addr text, p_players int) returns void
language plpgsql as $$
begin
  insert into world_sessions (world_id, status, host_addr, player_count, last_heartbeat_at)
  values (p_id, 'online', p_addr, p_players, now())
  on conflict (world_id) do update
     set status = 'online', host_addr = p_addr, player_count = p_players,
         last_heartbeat_at = now();
end; $$;
revoke execute on function heartbeat_world from public, anon, authenticated;
grant execute on function heartbeat_world to service_role;

-- ── 서버/오케스트레이터: 오프라인 전환 = 세션 행 삭제 ──
create or replace function set_world_offline(p_id bigint) returns void
language plpgsql as $$
begin
  delete from world_sessions where world_id = p_id;
end; $$;
revoke execute on function set_world_offline from public, anon, authenticated;
grant execute on function set_world_offline to service_role;

-- ── 오케스트레이터: 신호 끊긴 세션 회수 ──
--   requested 인데 임대 갱신 끊김 = 대기 클라가 떠남 (취소/종료/타임아웃 전부)
--   online    인데 하트비트 끊김 = 서버 죽음 (크래시/재부팅)
--   starting  는 건드리지 않음 — 부팅 중 서버는 아직 하트비트 전일 수 있어 오케스트레이터가 별도 유예
create or replace function reap_stale_sessions(p_request_timeout int, p_heartbeat_timeout int)
returns void language plpgsql as $$
begin
  delete from world_sessions
   where status = 'requested'
     and last_waiting_at < now() - make_interval(secs => p_request_timeout);

  delete from world_sessions
   where status = 'online'
     and coalesce(last_heartbeat_at, requested_at) < now() - make_interval(secs => p_heartbeat_timeout);
end; $$;
revoke execute on function reap_stale_sessions from public, anon, authenticated;
grant execute on function reap_stale_sessions to service_role;

-- ── 원자 배정: requested 월드 하나를 가장 오래된 미배정 스탠바이에 붙인다 ──
-- 반환 = 배정된 standby id (null = 가용 스탠바이 없음 → 호출측이 콜드 스폰 폴백)
create or replace function assign_standby_to_world(p_world_id bigint) returns bigint
language plpgsql as $$
declare v_standby_id bigint;
begin
  update world_sessions set status = 'starting'
   where world_id = p_world_id and status = 'requested';
  if not found then return null; end if;

  select id into v_standby_id from standby_servers
   where assigned_world_id is null
   order by created_at
   limit 1
   for update skip locked;
  if v_standby_id is null then return null; end if;   -- starting 으로 남고 오케스트레이터가 콜드 스폰

  update standby_servers set assigned_world_id = p_world_id where id = v_standby_id;
  return v_standby_id;
end; $$;
revoke execute on function assign_standby_to_world from public, anon, authenticated;
grant execute on function assign_standby_to_world to service_role;

-- ── 월드 삭제 (soft delete) — 소유자 본인 + 활성 세션 없을 때만 ──
create or replace function delete_world(p_id bigint) returns boolean
language plpgsql security definer set search_path = public as $$
declare v_updated int;
begin
  update worlds
     set deleted_at = now()
   where id = p_id
     and owner_account_id = auth.uid()
     and deleted_at is null
     and not exists (select 1 from world_sessions where world_id = p_id);  -- 가동 중이면 불가
  get diagnostics v_updated = row_count;
  return v_updated > 0;
end; $$;
revoke execute on function delete_world from public, anon;
grant execute on function delete_world to authenticated, service_role;
