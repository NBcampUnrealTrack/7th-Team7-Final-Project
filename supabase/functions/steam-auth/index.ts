// GY Steam 신원확인 — Steam 티켓 → GoTrue 세션 교환.
// UE 클라가 { steam_id, ticket?, persona_name? } 을 POST → access/refresh 토큰 + account_id 반환.
// STEAM_AUTH_MODE=stub  : steam_id 를 그대로 신뢰 (자체 App ID 확보 전까지의 dev 모드)
// STEAM_AUTH_MODE=verify: Steam Web API 로 티켓 실검증 (STEAM_WEB_API_KEY, STEAM_APP_ID 필요)
import { createClient, type SupabaseClient } from "npm:@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;
const ANON_KEY = Deno.env.get("SUPABASE_ANON_KEY")!;
const AUTH_MODE = Deno.env.get("STEAM_AUTH_MODE") ?? "stub";

type Account = {
    id: string;
    steam_id: string;
    persona_name: string | null
};

function jsonResponse(status: number, body: unknown): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

function steamEmail(steamId: string): string {
  return `${steamId}@steam.gy.invalid`;
}

async function resolveSteamId(steamId: string | undefined, ticket: string | undefined): Promise<string | null> {
  if (AUTH_MODE !== "verify") {
    // TODO: 자체 App ID(Steam Direct) 확보 후 verify 모드만 허용 — stub 은 steam_id 사칭 가능
    return steamId ?? null;
  }

  if (!ticket) return null;
  const url = new URL("https://api.steampowered.com/ISteamUserAuth/AuthenticateUserTicket/v1/");
  url.searchParams.set("key", Deno.env.get("STEAM_WEB_API_KEY")!);
  url.searchParams.set("appid", Deno.env.get("STEAM_APP_ID")!);
  url.searchParams.set("ticket", ticket);

  const res = await fetch(url);
  if (!res.ok) {
    console.error(`steam web api http ${res.status}`);
    return null;
  }
  const params = (await res.json())?.response?.params;
  if (params?.result !== "OK") {
    console.error("steam ticket rejected", params);
    return null;
  }
  // TODO: ownersteamid 불일치(family sharing) / vacbanned / publisherbanned 정책 — verify 전환 때 결정
  return params.steamid as string;
}

async function findAccount(admin: SupabaseClient, steamId: string): Promise<Account | null> {
  const { data } = await admin.from("accounts")
    .select("id, steam_id, persona_name").eq("steam_id", steamId).maybeSingle();
  return data;
}

// 없으면 GoTrue 유저 + accounts 행 생성. 생성 단계의 실패는 전부 동시 로그인 레이스로 보고 재조회로 수렴
async function ensureAccount(admin: SupabaseClient, steamId: string, personaName: string | null): Promise<Account | null> {
  const existing = await findAccount(admin, steamId);
  if (existing) {
    const newPersona = personaName ?? existing.persona_name;
    await admin.from("accounts")
      .update({ persona_name: newPersona, last_login_at: new Date().toISOString() })
      .eq("id", existing.id);
    return { ...existing, persona_name: newPersona };
  }

  const { data: created, error: createError } = await admin.auth.admin.createUser({
    email: steamEmail(steamId),
    email_confirm: true,
    user_metadata: { steam_id: steamId },
  });
  if (createError) {
    console.error("createUser failed (race → refetch)", createError);
    return await findAccount(admin, steamId);
  }

  const { data: inserted, error: insertError } = await admin.from("accounts")
    .insert({ id: created.user.id, steam_id: steamId, persona_name: personaName })
    .select("id, steam_id, persona_name").single();
  if (insertError) {
    console.error("accounts insert failed (race → refetch)", insertError);
    return await findAccount(admin, steamId);
  }
  return inserted;
}

// magiclink 의 token_hash 를 즉시 소모 — 비밀번호 없이 정식 GoTrue 세션 획득
async function mintSession(admin: SupabaseClient, email: string) {
  const { data: linkData, error: linkError } = await admin.auth.admin.generateLink({ type: "magiclink", email });
  if (linkError || !linkData?.properties?.hashed_token) {
    console.error("generateLink failed", linkError);
    return null;
  }

  const anonClient = createClient(SUPABASE_URL, ANON_KEY, { auth: { persistSession: false } });
  const { data: verified, error: verifyError } = await anonClient.auth.verifyOtp({
    type: "magiclink",
    token_hash: linkData.properties.hashed_token,
  });
  if (verifyError || !verified?.session) {
    console.error("verifyOtp failed", verifyError);
    return null;
  }
  return verified.session;
}

Deno.serve(async (req) => {
  if (req.method !== "POST") return jsonResponse(405, { error: "method not allowed" });

  let body: { steam_id?: string; ticket?: string; persona_name?: string };
  try {
    body = await req.json();
  } catch {
    return jsonResponse(400, { error: "invalid json" });
  }

  const steamId = await resolveSteamId(body.steam_id, body.ticket);
  if (!steamId) return jsonResponse(401, { error: "steam auth failed" });

  const personaName = body.persona_name?.slice(0, 50) ?? null;
  const admin = createClient(SUPABASE_URL, SERVICE_ROLE_KEY, { auth: { persistSession: false } });

  const account = await ensureAccount(admin, steamId, personaName);
  if (!account) return jsonResponse(500, { error: "account creation failed" });

  const session = await mintSession(admin, steamEmail(steamId));
  if (!session) return jsonResponse(500, { error: "session mint failed" });

  return jsonResponse(200, {
    account_id: account.id,
    steam_id: steamId,
    persona_name: account.persona_name,
    access_token: session.access_token,
    refresh_token: session.refresh_token,
    expires_in: session.expires_in,
  });
});
