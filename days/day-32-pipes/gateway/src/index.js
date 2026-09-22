// Day 31 — the Pipes gateway: the device's ambassador to WorkOS.
// The board authenticates with its AuthKit access token (the day-30
// prize); this Worker verifies it against the AuthKit JWKS, then acts
// with the WorkOS API key — which never leaves Cloudflare.
//
// Design: the device is deliberately dumb. One endpoint returns a
// display-ready screen — every provider with its connection state, a
// pre-minted authorization URL for the unconnected ones (the device
// just QRs it), and a pre-fetched detail line for the connected ones
// (looked up via Pipes Relay, in here, where the provider token never
// reaches the device). Features evolve by redeploying this Worker;
// the firmware never changes.
import { createRemoteJWKSet, jwtVerify } from "jose";

const WORKOS = "https://api.workos.com";

// How to describe a connected provider in one line, per slug. Add an
// entry, redeploy, and the device learns a new trick without a reflash.
const DETAILS = {
  github: {
    path: "/relay/github/user",
    line: (d) => `@${d.login}`,
  },
  linear: {
    path: "/relay/linear/graphql",
    method: "POST",
    body: JSON.stringify({ query: "{ viewer { name } }" }),
    line: (d) => d.data?.viewer?.name || "connected",
  },
};

let jwks = null;

async function requireUser(request, env) {
  const token = (request.headers.get("Authorization") || "").replace(
    /^Bearer\s+/i,
    ""
  );
  if (!token) throw new Response("missing token", { status: 401 });
  jwks ??= createRemoteJWKSet(
    new URL(`https://${env.AUTHKIT_DOMAIN}/oauth2/jwks`)
  );
  try {
    const { payload } = await jwtVerify(token, jwks);
    return { userId: payload.sub, orgId: payload.org_id || null };
  } catch {
    throw new Response("invalid token", { status: 401 });
  }
}

const workosHeaders = (env, extra = {}) => ({
  Authorization: `Bearer ${env.WORKOS_API_KEY}`,
  "Content-Type": "application/json",
  ...extra,
});

const bounded = (promise, ms, fallback) =>
  Promise.race([
    promise,
    new Promise((resolve) => setTimeout(() => resolve(fallback), ms)),
  ]);

async function detailFor(env, userId, slug) {
  const spec = DETAILS[slug];
  if (!spec) return "connected";
  const r = await fetch(`${WORKOS}${spec.path}`, {
    method: spec.method || "GET",
    headers: workosHeaders(env, { "X-Relay-User": userId }),
    body: spec.body,
    signal: AbortSignal.timeout(5000),
  }).catch(() => null);
  if (!r) return "relay timeout";
  if (!r.ok) return `relay ${r.status}`;
  try {
    return spec.line(await r.json());
  } catch {
    return "connected";
  }
}

async function connectUrlFor(env, userId, slug) {
  const r = await fetch(`${WORKOS}/data-integrations/${slug}/authorize`, {
    method: "POST",
    headers: workosHeaders(env),
    body: JSON.stringify({ user_id: userId }),
  });
  if (!r.ok) return null;
  return (await r.json()).url || null;
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname !== "/screen")
      return new Response("esptember pipes gateway", { status: 200 });

    let user;
    try {
      user = await requireUser(request, env);
    } catch (resp) {
      return resp;
    }
    const { userId, orgId } = user;

    // The per-user provider list, connections included — the same
    // endpoint auth.chan.dev uses in production. Connections are
    // workspace-scoped: the organization_id from the JWT matters.
    const qs = new URLSearchParams({ supports_multiple_connections: "true" });
    if (orgId) qs.set("organization_id", orgId);
    const r = await fetch(
      `${WORKOS}/user_management/users/${userId}/data_providers?${qs}`,
      { headers: workosHeaders(env) }
    );
    if (!r.ok) return new Response(await r.text(), { status: r.status });
    const body = await r.json();

    const list = Array.isArray(body) ? body : body.data || [];
    const providers = await Promise.all(
      list.map(async (d) => {
        const accounts = d.connected_accounts || [];
        const connected = accounts.some((a) => a.state === "connected");
        return {
          slug: d.slug,
          connected,
          detail: connected
            ? await bounded(detailFor(env, userId, d.slug), 6000, "connected")
            : null,
          connect_url: connected
            ? null
            : await bounded(connectUrlFor(env, userId, d.slug), 6000, null),
        };
      })
    );
    return Response.json({ user: userId, providers });
  },
};
