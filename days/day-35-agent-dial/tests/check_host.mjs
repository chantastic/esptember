#!/usr/bin/env node
// Host bridge checks for a Day 35 host candidate, with no board and no harness.
// Usage: node tests/check_host.mjs <host-candidate-dir>
import fs from "node:fs";
import os from "node:os";
import net from "node:net";
import path from "node:path";
import assert from "node:assert/strict";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const HOST = path.resolve(process.argv[2] ?? path.join(HERE, "..", ".build", "host-candidate", "agent-dial"));
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), "agent-dial-test-"));
Object.assign(process.env, {
  AGENT_DIAL_HOME: TMP, AGENT_DIAL_SOCKET: path.join(TMP, "hub.sock"), AGENT_DIAL_PORT: "none",
  AGENT_DIAL_NO_SPAWN: "1", AGENT_DIAL_STT: "echo '  hello   dial '",
});
const load = (rel) => import(pathToFileURL(path.join(HOST, rel)).href);
const frame = await load("lib/frame.mjs");
const { redact } = await load("lib/redact.mjs");
const hub = await load("hub.mjs");
const cli = await load("bin/agent-dial.mjs");

let checks = 0;
const ok = (cond, msg) => { assert.ok(cond, msg); checks++; };
const eq = (a, b, msg) => { assert.deepEqual(a, b, msg); checks++; };
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// ---- codec against the shared vectors ----
for (const line of fs.readFileSync(path.join(HERE, "vectors", "dial_frame.txt"), "utf8").split("\n")) {
  if (!line || line.startsWith("#")) continue;
  const f = line.split("\t");
  if (f[0] === "encode") { eq(frame.encode(Number(f[2]), Buffer.from(f[3], "hex")).toString("hex"), f[4], `encode ${f[1]}`); continue; }
  const got = [];
  const d = new frame.Decoder((t, p) => got.push(`${t}:${p.toString("hex")}`));
  const data = Buffer.from(f[2], "hex");
  for (let i = 0; i < data.length; i += 3) d.feed(data.subarray(i, i + 3));
  eq(got.join(",") || "-", f[3], `stream ${f[1]}`);
  eq([d.discarded, d.corrupt], [Number(f[4]), Number(f[5])], `counters ${f[1]}`);
}
ok(frame.crc16(Buffer.from("123456789")) === 0x29b1, "CRC check value");

// ---- redaction ----
for (const c of JSON.parse(fs.readFileSync(path.join(HERE, "redaction_cases.json"), "utf8"))) {
  const out = redact(c.in);
  if (c.equals) eq(out, c.equals, `over-redacted: ${c.in}`);
  else ok(!out.includes(c.must_not_contain), `leaked: ${c.in} -> ${out}`);
}

// ---- hub with a fake board ----
hub.main();
for (let i = 0; i < 50 && !fs.existsSync(process.env.AGENT_DIAL_SOCKET); i++) await sleep(20);
let sent = [];
const device = () => hub.attachDevice((buf) => {
  const d = new frame.Decoder((t, p) => { if (t === frame.TYPE.JSON) sent.push(JSON.parse(p.toString("utf8"))); });
  d.feed(buf);
  ok(buf.length - 8 < 2048, `frame to dial under 2048 bytes (${buf.length})`);
});
const take = (t) => { const hits = sent.filter((m) => m.t === t); sent = sent.filter((m) => m.t !== t); return hits; };
const say = (dev, msg) => dev.feed(frame.encodeJson(msg));
function adapter() {
  const s = net.createConnection(process.env.AGENT_DIAL_SOCKET);
  s.setEncoding("utf8");
  s.got = [];
  let buf = "";
  s.on("data", (d) => { buf += d; let i; while ((i = buf.indexOf("\n")) >= 0) { s.got.push(JSON.parse(buf.slice(0, i))); buf = buf.slice(i + 1); } });
  s.send = (m) => s.write(JSON.stringify(m) + "\n");
  return new Promise((r) => s.once("connect", () => r(s)));
}

// A foreign board is left alone.
let dev = device();
say(dev, { t: "hello", product: "harness", proto: 3, mac: "AA:BB:CC:DD:EE:01", fw: "v1" });
await sleep(20);
ok(!dev.attached, "foreign product releases the port");
eq(take("welcome"), [], "no welcome for a foreign product");

// Our board: welcome, and pairing with a mode-600 record.
dev = device();
say(dev, { t: "hello", product: "agent-dial", proto: 1, hw: "m5stack-stopwatch", mac: "AA:BB:CC:DD:12:34", fw: "0.1", wifi: "connected", ip: "192.168.0.9", paired: false, key: "-" });
await sleep(20);
eq(take("welcome").length, 1, "welcome");
const pairMsg = take("pair");
eq(pairMsg.length, 1, "unpaired board is paired");
ok(/^[0-9a-f]{64}$/.test(pairMsg[0].token), "token is 64 hex");
const rec = JSON.parse(fs.readFileSync(path.join(TMP, "device.json"), "utf8"));
eq([rec.token, rec.ip, rec.host, rec.port], [pairMsg[0].token, "192.168.0.9", "agent-dial-1234.local", 47838], "pairing record");
eq(fs.statSync(path.join(TMP, "device.json")).mode & 0o777, 0o600, "pairing record is private");

// Sessions merge; fields are capped; bad ids are ignored.
const pi = await adapter();
pi.send({ t: "hello", id: "pi:abc", name: "x".repeat(100), harness: "pi", cwd: "/w/esptember", caps: ["stop", "prompt", "bogus"] });
pi.send({ t: "hello", id: "bad id with spaces", harness: "pi" });
await sleep(40);
const agents = take("agent");
eq(agents.length, 1, "one agent pushed");
eq([agents[0].id, agents[0].name.length, agents[0].harness, agents[0].caps], ["pi:abc", 39, "pi", ["stop", "prompt"]], "agent fields");
ok(take("agents.end").at(-1).total === 1, "agents.end total");
pi.send({ t: "turn.started", id: "pi:abc", text: "$ " + "y".repeat(200) });
pi.send({ t: "summary", id: "pi:abc", text: "z".repeat(500) });
await sleep(20);
eq(take("turn.started")[0].text.length, 60, "turn text cap");
eq(take("summary")[0].text.length, 160, "summary cap");

// Capabilities gate what may be asked and commanded.
pi.send({ t: "question", id: "pi:abc", requestId: "r0", questions: [{ key: "decision", q: "Allow?", options: ["Allow", "Deny"] }] });
await sleep(20);
eq(take("question"), [], "no approval without approve");
say(dev, { t: "turn.stop", id: "pi:abc" });
await sleep(20);
ok(pi.got.some((m) => m.t === "turn.stop" && m.id === "pi:abc"), "turn.stop reaches a session with stop");

const cliA = await adapter();
cliA.send({ t: "hello", id: "cli:ask", harness: "cli", caps: ["answer"] });
cliA.send({ t: "question", id: "cli:ask", requestId: "r1", questions: Array.from({ length: 6 }, (_, i) => ({ key: `k${i}`, q: "Q".repeat(300), options: Array.from({ length: 9 }, (_, j) => "o".repeat(120) + j) })) });
await sleep(40);
const q = take("question")[0];
ok(q.questions.length >= 1 && q.questions.every((x) => x.options.length === 6 && Array.from(x.q).length <= 199 && x.options.every((o) => Array.from(o).length <= 79)), "question caps");
ok(Buffer.byteLength(JSON.stringify(q)) < 2000, "an oversized question is shortened to fit");
const small = { t: "question", id: "cli:ask", requestId: "r1b", questions: [{ key: "k", q: "Q".repeat(300), options: ["o".repeat(120), "p"] }] };
cliA.send(small);
await sleep(20);
const sq = take("question")[0];
eq([Array.from(sq.questions[0].q).length, Array.from(sq.questions[0].options[0]).length], [199, 79], "a question that fits keeps full caps");
say(dev, { t: "answer", id: "cli:ask", requestId: "r1b", answers: { k: "p" } });
say(dev, { t: "turn.stop", id: "cli:ask" });
say(dev, { t: "answer", id: "cli:ask", requestId: "r1", answers: { k0: "o1", k1: 7 } });
await sleep(20);
ok(!cliA.got.some((m) => m.t === "turn.stop"), "no turn.stop without stop");
eq(cliA.got.find((m) => m.t === "answer" && m.requestId === "r1"), { t: "answer", id: "cli:ask", requestId: "r1", answers: { k0: "o1" } }, "answer routed, strings only");
eq(hub.statusMsg().questions, [], "answered question is gone");

// An adapter that exits takes its question and its socket-held session with it.
const gone = await adapter();
gone.send({ t: "hello", id: "cli:gone", harness: "cli", caps: ["approve"] });
gone.send({ t: "question", id: "cli:gone", requestId: "r2", questions: [{ key: "decision", q: "Allow?", options: ["Allow", "Deny"] }] });
await sleep(30);
eq(take("question").length, 1, "approval forwarded");
take("agent");
gone.destroy();
await sleep(40);
eq(take("question.close"), [{ t: "question.close", id: "cli:gone", requestId: "r2" }], "question closed when its adapter exits");
ok(!take("agent").some((a) => a.id === "cli:gone"), "socket-held session removed on exit");

// A ttl session outlives its connection, then expires.
const shortLived = await adapter();
shortLived.send({ t: "hello", id: "claude:t1", harness: "claude", caps: ["approve"], ttl: 1 });
await sleep(30);
shortLived.destroy();
await sleep(30);
ok(hub.statusMsg().sessions.some((s) => s.id === "claude:t1"), "ttl session survives its connection");
await sleep(1100);
hub.tick();
ok(!hub.statusMsg().sessions.some((s) => s.id === "claude:t1"), "ttl session expires");

// Voice: addressed, routed to the most recent prompt session, and refused without one.
take("voice.transcript");
say(dev, { t: "voice.begin", id: "pi:abc", lang: "en", sr: 16000 });
dev.feed(frame.encode(frame.TYPE.PCM, Buffer.alloc(1024)));
say(dev, { t: "voice.end" });
await sleep(300);
ok(pi.got.some((m) => m.t === "turn.send" && m.id === "pi:abc" && m.text === "hello dial"), "voice becomes turn.send");
eq(take("voice.transcript")[0]?.id, "pi:abc", "transcript names the session");
say(dev, { t: "voice.begin", lang: "en", sr: 16000 });
say(dev, { t: "voice.end" });
await sleep(300);
eq(take("voice.transcript")[0]?.id, "pi:abc", "routed take goes to the prompt session");
say(dev, { t: "voice.begin", id: "cli:ask", lang: "en", sr: 16000 });
say(dev, { t: "voice.end" });
await sleep(300);
eq(take("voice.error")[0]?.message, "No agent to send that to", "no prompt capability, no route");

// Claude Code: an approval decided on the dial, and one closed by the terminal.
const input = { session_id: "5e55-10n", cwd: "/w/app", tool_name: "Bash", tool_input: { command: "rm -rf build TOKEN=abc" }, tool_use_id: "toolu_1" };
const pending = cli.claudeHook("PermissionRequest", input);
await sleep(150);
const cq = take("question")[0];
eq([cq.id, cq.requestId, cq.questions[0].key, cq.questions[0].options], ["claude:5e55-10n", "toolu_1", "decision", ["Allow", "Deny"]], "Claude approval question");
ok(!cq.questions[0].q.includes("abc"), "approval text is redacted");
ok(hub.statusMsg().sessions.find((s) => s.id === "claude:5e55-10n")?.caps.join() === "approve", "Claude declares only approve");
say(dev, { t: "answer", id: "claude:5e55-10n", requestId: "toolu_1", answers: { decision: "Deny" } });
eq(await pending, { hookSpecificOutput: { hookEventName: "PermissionRequest", decision: { behavior: "deny", message: "Denied on the Agent Dial" } } }, "deny decision");
const pending2 = cli.claudeHook("PermissionRequest", { ...input, tool_use_id: "toolu_2" });
await sleep(150);
await cli.claudeHook("PostToolUse", { ...input, tool_use_id: "toolu_2" });
eq(await pending2, null, "answered in the terminal: no decision, question closed");
ok(take("question.close").some((m) => m.requestId === "toolu_2"), "dial told the question closed");
const pending3 = cli.claudeHook("PermissionRequest", { ...input, tool_use_id: "toolu_3" });
await sleep(150);
say(dev, { t: "answer", id: "claude:5e55-10n", requestId: "toolu_3", answers: { decision: "Allow" } });
eq((await pending3)?.hookSpecificOutput?.decision, { behavior: "allow" }, "allow decision");
await cli.claudeHook("Stop", { ...input, last_assistant_message: "Set PASSWORD=hunter2 and done." });
await sleep(30);
ok(!take("summary").at(-1).text.includes("hunter2"), "Claude recap is redacted");

// Reboot: the dial asks for the list and gets open state back.
cliA.send({ t: "question", id: "cli:ask", requestId: "r9", questions: [{ key: "choice", q: "Pick", options: ["A", "B"] }] });
await sleep(20);
take("question"); take("agents.end");
say(dev, { t: "agents.list" });
await sleep(30);
ok(take("agents.end").length === 1 && take("question").some((m) => m.requestId === "r9"), "resync resends open questions");

console.log(`agent-dial host bridge: ${checks} checks passed.`);
fs.rmSync(TMP, { recursive: true, force: true });
process.exit(0);
