-- ============================================================
-- 월드 삭제 (soft delete) — 소유자 본인 + offline 상태만.
-- deleted_at 마킹이라 세이브 데이터는 남는다 (worlds_select 정책과 생성 제한 트리거가
-- deleted_at is null 만 세므로, 목록 제거와 생성 슬롯 반환은 자동).
-- ============================================================

create or replace function delete_world(p_id bigint) returns boolean
language plpgsql security definer set search_path = public as $$
declare v_updated int;
begin
  update worlds
     set deleted_at = now()
   where id = p_id
     and owner_account_id = auth.uid() -- 소유자 본인만 (시스템 월드는 owner null 이라 자동 차단)
     and deleted_at is null
     and status = 'offline';           -- 가동 중인 월드는 서버가 내려간 뒤에만
  get diagnostics v_updated = row_count;
  return v_updated > 0;
end; $$;

revoke execute on function delete_world from public, anon;
grant execute on function delete_world to authenticated, service_role;
