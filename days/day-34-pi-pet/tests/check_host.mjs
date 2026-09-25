#!/usr/bin/env node
// Portable host-bridge checks (no board, no pi): activity redaction and the
// hub's session merge over a temporary socket. Usage:
//   node check_host.mjs <host-candidate-dir>
import { spawn } from "node:child_process";
import fs from "node:fs";
import net from "node:net";
import os from "node:os";
import path from "node:path";
import assert from "node:assert/strict";

const dir = path.resolve(process.argv[2] ?? "");
const here = path.dirname(new URL(import.meta.url).pathname);
let checks = 0;
const ok = (cond, msg) => { assert.ok(cond, msg); checks++; };

// 1. Redaction: the extension exports redact(text).
const { redact } = await import(path.join(dir, "index.ts"));
for (const c of JSON.parse(fs.readFileSync(path.join(here, "redaction_cases.json"), "utf8"))) {
  const out = redact(c.in);
  if (c.must_not_contain) ok(!out.includes(c.must_not_contain), `leaks secret: ${out}`);
  if (c.equals) ok(out === c.equals, `over-redacted: ${out}`);
}

// 2. Hub merge semantics.
const sock = path.join(os.tmpdir(), `pi-pet-test-${process.pid}.sock`);
const hub = spawn(process.execPath, [path.join(dir, "hub.mjs")], {
  env: { ...process.env, PI_PET_SOCKET: sock, PI_PET_PORT: "none" }, stdio: "ignore",
});
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const client = async () => {
  for (let i = 0; i < 50; i++) {
    const c = await new Promise((res) => {
      const s = net.createConnection(sock);
      s.on("connect", () => res(s));
      s.on("error", () => res(null));
    });
    if (c) { c.setEncoding("utf8"); return c; }
    await sleep(100);
  }
  throw new Error("hub did not start");
};
const send = (c, m) => c.write(JSON.stringify(m) + "\n");
const status = async () => {
  const c = await client();
  const reply = new Promise((res) => c.once("data", (d) => res(JSON.parse(String(d).split("\n")[0]))));
  send(c, { t: "status" });
  const st = await reply;
  c.end();
  return st;
};
try {
  const a = await client(), b = await client();
  send(a, { t: "hello", id: "session-aaaaaaaa1111", name: "esptember", cwd: "/x/esptember" });
  send(b, { t: "hello", id: "session-bbbbbbbb2222", name: "caf\u00e9 | pipes", cwd: "/x/b" });
  send(a, { t: "state", id: "session-aaaaaaaa1111", state: "tool", detail: "$ make" });
  send(b, { t: "state", id: "session-bbbbbbbb2222", state: "exploding", detail: "" });
  await sleep(200);
  let st = await status();
  ok(st.pets.length === 2, "two sessions become two pets");
  const pa = st.pets.find((p) => p.name === "esptember"), pb = st.pets.find((p) => p !== pa);
  ok(pa && pa.id === "aaaa1111", "ids are the last 8 alphanumerics");
  ok(pa.state === "tool" && pa.detail === "$ make", "state and detail are merged");
  ok(pb.state === "idle", "unknown states fall back to idle");
  ok(/^[\x20-\x7e]*$/.test(pb.name) && !pb.name.includes("|"), "names are printable ASCII without the field separator");
  ok(pa.style !== pb.style, "live pets get distinct styles");
  b.destroy();  // a crashed session takes its pet with it
  await sleep(200);
  st = await status();
  ok(st.pets.length === 1 && st.pets[0].id === "aaaa1111", "socket close removes that session's pet");
  send(a, { t: "bye", id: "session-aaaaaaaa1111" });
  await sleep(200);
  st = await status();
  ok(st.pets.length === 0, "bye removes the pet");
  for (let i = 0; i < 10; i++) send(a, { t: "hello", id: `session-xxxxxxx${i}`, name: `n${i}` });
  await sleep(200);
  st = await status();
  ok(st.pets.length === 8, "the roster is capped at eight pets");
  a.end();
} finally {
  hub.kill("SIGTERM");
  try { fs.unlinkSync(sock); } catch {}
}
console.log(`pi-pet host bridge: ${checks} checks passed.`);
