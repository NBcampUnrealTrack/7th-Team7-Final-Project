-- 월드 참여자 — "참가 중인 월드" 목록의 근거.
-- 참여 주체는 계정 (같은 계정이면 캐릭터를 바꿔도 같은 참여).
-- 주의: 참여는 "권한"이 아니라 노출용 소속 기록 — 입장 제한은 동시 4인(서버 PreLogin)뿐.
--   멤버 관리(초대/추방) 도입 시: 추방 = 행 삭제, 입장 통제가 필요하면 PreLogin 에서 명단 검사 추가.
-- 데디가 플레이어 입장 시 기록(service_role, 캐릭터 id 로 계정 resolve), 클라는 자기 행만 조회(RLS).

create table world_participants (
  world_id       bigint not null,
  account_id     uuid not null,
  last_played_at timestamptz not null default now(),
  primary key (world_id, account_id)
);

alter table world_participants enable row level security;
grant all on table world_participants to service_role;
grant select on table world_participants to authenticated;

create policy world_participants_select on world_participants
  for select using (account_id = auth.uid());

-- 데디 전용 upsert — 서버는 charId 만 알므로 계정은 여기서 resolve
create or replace function record_world_participant(p_world_id bigint, p_character_id bigint)
returns void language plpgsql as $$
declare v_account uuid;
begin
  select account_id into v_account from characters
   where id = p_character_id and deleted_at is null;
  if v_account is null then return; end if;

  insert into world_participants (world_id, account_id)
  values (p_world_id, v_account)
  on conflict (world_id, account_id) do update set last_played_at = now();
end; $$;
revoke execute on function record_world_participant from public, anon, authenticated;
grant execute on function record_world_participant to service_role;

-- 생성 = 자동 참여 (불변식): 소유 계정이 있는 월드가 만들어지면 참여 기록도 함께 생성
create or replace function record_owner_participant()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  if new.owner_account_id is not null then
    insert into world_participants (world_id, account_id)
    values (new.id, new.owner_account_id)
    on conflict do nothing;
  end if;
  return new;
end; $$;
create trigger trg_world_owner_participant
  after insert on worlds
  for each row execute function record_owner_participant();
