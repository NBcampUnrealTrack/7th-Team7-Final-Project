-- ============================================================
-- 클라이언트 월드 생성 — 소유/참여 주체는 "계정" (캐릭터는 계정 안에서 교체 가능).
-- 이름/호스트표기만 지정, 나머지(레벨/상태/세이브)는 기본값.
-- ============================================================

alter table worlds add column owner_name varchar(50);   -- 카드 "호스트: 이름" (생성 시점 persona denorm)

grant insert (name, owner_account_id, owner_name) on table worlds to authenticated;

create policy worlds_insert on worlds
  for insert with check (owner_account_id = auth.uid());

-- 계정당 "생성(소유)" 개수 제한 — 입장은 무제한 (입장 제한은 월드당 4인, 서버 PreLogin 담당).
-- 목적: 행 무한 생성으로 인한 목록 오염/스폰 요청 남발 방지 백스톱. 시스템 월드(owner null)는 카운트 제외
create or replace function enforce_world_limit()
returns trigger language plpgsql as $$
declare v_count int;
begin
  if new.owner_account_id is not null then
    select count(*) into v_count from worlds
     where owner_account_id = new.owner_account_id and deleted_at is null;
    if v_count >= 2 then                 -- 정책값: 계정당 최대 월드 수
      raise exception 'world limit reached';
    end if;
  end if;
  return new;
end; $$;
create trigger trg_world_limit
  before insert on worlds
  for each row execute function enforce_world_limit();
