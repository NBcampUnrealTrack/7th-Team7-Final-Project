-- ============================================================
-- 월드 세션 목록(서버 하트비트 등록, 클라 조회) + 온디맨드 스폰 큐
-- worlds 행이 "세션 존재 증명"을 겸한다: 서버가 하트비트로 자기 주소/인원을 갱신,
-- 클라는 목록을 읽고, 오프라인 월드는 시작 요청(start_requested_at)으로 오케스트레이터가 스폰.
-- ============================================================

alter table worlds
  add column host_addr          text,                          -- "ip:port" (online 일 때만 유효)
  add column player_count       int not null default 0,
  add column max_players        int not null default 4,
  add column status             text not null default 'offline',  -- offline | starting | online
  add column heartbeat_at       timestamptz,
  add column start_requested_at timestamptz;

-- 클라(authenticated): 월드 목록 조회 허용 (팀/MVP 단계 — 전체 공개 목록)
grant select on table worlds to authenticated;
create policy worlds_select on worlds
  for select using (deleted_at is null);

-- ── 클라: 시작 요청 (오프라인 월드만 큐잉 — 중복 요청은 타임스탬프 갱신일 뿐 무해) ──
create or replace function request_world_start(p_id bigint) returns boolean
language plpgsql security definer set search_path = public
as $$
begin
  update worlds
     set start_requested_at = now()
   where id = p_id and deleted_at is null and status = 'offline';
  return found;
end; $$;
revoke execute on function request_world_start from public, anon;
grant execute on function request_world_start to authenticated, service_role;

-- ── 서버: 하트비트 (등록 겸용 — 첫 하트비트가 online 전환) ──
create or replace function heartbeat_world(p_id bigint, p_addr text, p_players int) returns void
language plpgsql
as $$
begin
  update worlds
     set status = 'online', host_addr = p_addr, player_count = p_players,
         heartbeat_at = now(), start_requested_at = null
   where id = p_id and deleted_at is null;
end; $$;
revoke execute on function heartbeat_world from public, anon, authenticated;
grant execute on function heartbeat_world to service_role;

-- ── 서버/오케스트레이터: 오프라인 전환 ──
create or replace function set_world_offline(p_id bigint) returns void
language plpgsql
as $$
begin
  update worlds
     set status = 'offline', host_addr = null, player_count = 0
   where id = p_id and deleted_at is null;
end; $$;
revoke execute on function set_world_offline from public, anon, authenticated;
grant execute on function set_world_offline to service_role;

-- ── 오케스트레이터: 스폰 클레임 (offline+요청 있는 행을 starting 으로 원자 전환) ──
-- 반환 true = 이 호출자가 스폰 권리를 가짐. 두 에이전트가 경합해도 한쪽만 true
create or replace function claim_world_start(p_id bigint) returns boolean
language plpgsql
as $$
begin
  update worlds
     set status = 'starting'
   where id = p_id and deleted_at is null and status = 'offline'
     and start_requested_at is not null;
  return found;
end; $$;
revoke execute on function claim_world_start from public, anon, authenticated;
grant execute on function claim_world_start to service_role;
