// pi-pet: report this pi session's activity to the Stopwatch pet hub.
// Never throws into pi; if the hub or board is missing, it quietly retries.
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import net from "node:net";
import path from "node:path";
import os from "node:os";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";

const SOCKET_PATH = process.env.PI_PET_SOCKET || path.join(os.homedir(), ".pi-pet", "hub.sock");
const HUB_PATH = path.join(path.dirname(fileURLToPath(import.meta.url)), "hub.mjs");

type PetState = "idle" | "thinking" | "tool" | "error" | "done" | "waiting" | "sleeping";

// The activity line is visible to anyone near the desk: redact obvious secrets,
// and PI_PET_DETAIL=0 reduces it to the tool name.
const SECRET = /\b([A-Za-z_]*(?:token|secret|password|passwd|api[_-]?key|auth)[A-Za-z_]*)(\s*[=:]\s*|\s+)("[^"]*"|'[^']*'|\S+)/gi;
export function redact(text: string): string {
  return text.replace(/\b(Bearer|Basic)\s+[^\s"']+/gi, "$1 ***").replace(SECRET, (m, k, sep, v) => (v === "***" || /^(Bearer|Basic)$/i.test(v) ? m : `${k}${sep}***`));
}

function toolDetail(name: string, args: any): string {
  if (process.env.PI_PET_DETAIL === "0") return name.replace(/^computer_use_/, "");
  try {
    const file = (p: unknown) => (typeof p === "string" ? path.basename(p) : "");
    switch (name) {
      case "bash": return redact(`$ ${String(args?.command ?? "").split("\n")[0]}`);
      case "read": return `reading ${file(args?.path)}`;
      case "edit": return `editing ${file(args?.path)}`;
      case "write": return `writing ${file(args?.path)}`;
      default: return name.replace(/^computer_use_/, "");
    }
  } catch {
    return name;
  }
}

export default function (pi: ExtensionAPI) {
  if (process.env.PI_PET === "0") return;

  let sock: net.Socket | null = null;
  let connecting = false;
  let spawned = false;
  let id = "";
  let name = "pi";
  let cwd = "";
  let last = { state: "idle" as PetState, detail: "" };
  let retry: ReturnType<typeof setTimeout> | undefined;
  let closed = false;

  const send = (msg: object) => {
    if (sock && !sock.destroyed) sock.write(JSON.stringify(msg) + "\n");
  };
  const hello = () => send({ t: "hello", id, name, cwd });
  const report = (state: PetState, detail = "") => {
    if (state === last.state && detail === last.detail) return;
    last = { state, detail };
    send({ t: "state", id, state, detail });
  };

  const connect = () => {
    if (closed || connecting || (sock && !sock.destroyed)) return;
    connecting = true;
    const s = net.createConnection(SOCKET_PATH);
    s.on("connect", () => {
      connecting = false;
      sock = s;
      hello();
      send({ t: "state", id, state: last.state, detail: last.detail });
    });
    s.on("error", () => {
      connecting = false;
      if (!spawned) {  // first failure: start the hub, then try again shortly
        spawned = true;
        try { spawn(process.execPath, [HUB_PATH], { detached: true, stdio: "ignore" }).unref(); } catch {}
      }
    });
    s.on("close", () => {
      connecting = false;
      if (sock === s) sock = null;
      if (!closed) { clearTimeout(retry); retry = setTimeout(connect, 1500); retry.unref?.(); }
    });
    s.on("data", () => {});  // focus/ack events are reserved for later (TTS, approvals)
  };

  pi.on("session_start", async (_event, ctx) => {
    try {
      closed = false;
      id = ctx.sessionManager.getSessionId?.() ?? `${process.pid}`;
      cwd = ctx.cwd;
      name = pi.getSessionName?.() || path.basename(cwd) || "pi";
      last = { state: "idle", detail: "" };
      connect();
    } catch {}
  });

  pi.on("session_info_changed", async (event) => {
    name = event.name || path.basename(cwd) || "pi";
    hello();
  });

  pi.on("agent_start", async () => report("thinking"));
  pi.on("turn_start", async () => { if (last.state !== "tool") report("thinking"); });
  pi.on("tool_execution_start", async (event) => report("tool", toolDetail(event.toolName, event.args)));
  pi.on("tool_execution_end", async (event) => {
    // A non-zero bash exit is a normal tool result in pi; the pet still winces.
    let failed = !!event.isError;
    try {
      const text = JSON.stringify((event.result as any)?.content ?? "");
      if (event.toolName === "bash" && /exited with code [1-9]/.test(text)) failed = true;
    } catch {}
    report(failed ? "error" : "thinking", failed ? `${event.toolName} failed` : "");
  });
  pi.on("agent_settled", async () => report(last.state === "error" ? "error" : "done", last.state === "error" ? last.detail : "your turn"));

  pi.on("session_shutdown", async () => {
    closed = true;
    clearTimeout(retry);
    send({ t: "bye", id });
    sock?.end();
    sock = null;
  });

  // One request/response over a fresh hub connection (status, wifi, forget).
  const ask = (msg: object, want: string, timeoutMs = 70_000) =>
    new Promise<any>((resolve) => {
      const s = net.createConnection(SOCKET_PATH);
      let buf = "";
      const done = (v: any) => { clearTimeout(t); s.destroy(); resolve(v); };
      const t = setTimeout(() => done(null), timeoutMs);
      s.setEncoding("utf8");
      s.on("connect", () => s.write(JSON.stringify(msg) + "\n"));
      s.on("data", (d) => {
        buf += d;
        for (const line of buf.split("\n")) {
          try { const m = JSON.parse(line); if (m.t === want) return done(m); } catch {}
        }
      });
      s.on("error", () => done(null));
    });

  pi.registerCommand("pet", {
    description: "pi-pet: status · `/pet wifi [ssid]` sends your Wi-Fi to the Stopwatch over USB · `/pet forget` unpairs · `/pet release` frees USB for flashing · `/pet hop|mood <name>` pokes the pet",
    handler: async (args, ctx) => {
      const a = (args ?? "").trim();
      if (a === "release") { send({ t: "release" }); ctx.ui.notify("pi-pet: USB released for 30 s (flash now; Wi-Fi stays up)", "info"); return; }
      if (a === "wifi" || a.startsWith("wifi ")) {
        ctx.ui.notify("pi-pet: reading the Wi-Fi password from your keychain (macOS may ask to allow)…", "info");
        const r = await ask({ t: "wifi", ssid: a.slice(4).trim() || undefined }, "result");
        ctx.ui.notify(`pi-pet: ${r ? r.message : "hub not running"}`, r?.ok ? "info" : "warning");
        return;
      }
      if (a === "forget") {
        const r = await ask({ t: "forget" }, "result", 5000);
        ctx.ui.notify(`pi-pet: ${r ? r.message : "hub not running"}`, r?.ok ? "info" : "warning");
        return;
      }
      if (a) { send({ t: "raw", line: a }); return; }
      const st = await ask({ t: "status" }, "status", 3000);
      if (!st) { ctx.ui.notify("pi-pet: hub not running", "warning"); return; }
      const links = (st.links ?? []).map((l: any) => `${l.kind}${l.active ? "*" : ""}`).join(" + ") || "no board";
      ctx.ui.notify(`pi-pet: ${links}${st.wifi ? ` · listening ${st.wifi.address}:${st.wifi.port}` : ""} · ${st.pets.length} pet(s): ` +
        st.pets.map((p: any) => `${p.name}=${p.state}`).join(", "), "info");
    },
  });
}
