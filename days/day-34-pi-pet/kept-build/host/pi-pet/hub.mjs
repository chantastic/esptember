#!/usr/bin/env node
// pi-pet hub: merges state reports from every running pi session and drives the Stopwatch
// over USB serial or the local Wi-Fi network.
//
//   pi extensions --JSON lines, ~/.pi-pet/hub.sock--> hub --USB serial or TCP (LAN)--> Stopwatch
//
// Client -> hub:  {"t":"hello","id","name","cwd"}  {"t":"state","id","state","detail"}
//                 {"t":"bye","id"}  {"t":"raw","line"}  {"t":"status"}  {"t":"monitor"}
//                 {"t":"release"}  {"t":"wifi","ssid"?}  {"t":"forget"}
// Hub -> client:  {"t":"status",...}  {"t":"device","line"}  {"t":"focus","id"}  {"t":"ack","id"}
//                 {"t":"result","ok","message"}
//
// USB is the trusted channel: whenever the board is on USB the hub pairs it (random 32-byte
// token, kept in ~/.pi-pet/devices.json) and learns its LAN address. Over Wi-Fi the hub dials
// the board (port 47837 at its last address, else pi-pet-<id4>.local): outgoing connections
// need no firewall approval on the Mac. Both sides prove knowledge of the token
// (HMAC-SHA256 over each side's nonce) before any display traffic.
import net from "node:net";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import crypto from "node:crypto";
import { spawn, execFile } from "node:child_process";
import { SerialPort } from "serialport";

export const HUB_DIR = process.env.PI_PET_HOME || path.join(os.homedir(), ".pi-pet");
export const SOCKET_PATH = process.env.PI_PET_SOCKET || path.join(HUB_DIR, "hub.sock");
const DEVICES_PATH = path.join(HUB_DIR, "devices.json");
const LOG_PATH = path.join(HUB_DIR, "hub.log");
const DEVICE_PORT = Number(process.env.PI_PET_DEVICE_PORT || 47837);
const WIFI_ENABLED = process.env.PI_PET_WIFI !== "0";
const IDLE_EXIT_MS = 5 * 60_000;
const STATES = new Set(["idle", "thinking", "tool", "error", "done", "waiting", "sleeping"]);
const STYLE_COUNT = 8;
const MAX_PETS = 8;

const log = (...args) => {
  const line = `${new Date().toISOString()} ${args.join(" ")}\n`;
  try { fs.appendFileSync(LOG_PATH, line); } catch {}
  if (process.stdout.isTTY) process.stdout.write(line);
};

// Device text protocol is space-delimited with a final "name|detail" field.
const clean = (s, max) => String(s ?? "").replace(/[^\x20-\x7e]/g, "?").replace(/\|/g, "/").trim().slice(0, max);
const shortId = (id) => clean(id, 64).replace(/[^A-Za-z0-9]/g, "").slice(-8).padStart(8, "0");
const hmac = (key, msg) => crypto.createHmac("sha256", key).update(msg).digest("hex");
const keyPrint = (token) => crypto.createHash("sha256").update(token).digest("hex").slice(0, 8);

const pets = new Map();     // id -> {id, name, state, detail, style, owner}
const clients = new Set();  // sockets
const monitors = new Set();
let lastClientAt = Date.now();
let releasedUntil = 0;
let pausedUntil = 0;  // tests: leave the board entirely alone (no USB, no Wi-Fi) for a while

// ---- devices and links ----
// A link is one transport to a board: {kind: "usb"|"wifi", deviceId, write(line), close()}.
// Each device uses one active link (USB preferred); the other stays connected on standby.
const links = new Set();
let usb = null;  // {link, port, path, rx}
function loadDevices() { try { return JSON.parse(fs.readFileSync(DEVICES_PATH, "utf8")); } catch { return {}; } }
function saveDevices(d) { fs.writeFileSync(DEVICES_PATH, JSON.stringify(d, null, 1), { mode: 0o600 }); }

function activeLinks() {
  const byDevice = new Map();
  for (const l of links) {
    if (!l.deviceId && l.kind === "usb") { byDevice.set("usb:" + (usb?.path ?? ""), l); continue; }
    if (!l.deviceId || !l.authed) continue;
    const cur = byDevice.get(l.deviceId);
    if (!cur || (cur.kind === "wifi" && l.kind === "usb")) byDevice.set(l.deviceId, l);
  }
  return new Set(byDevice.values());
}
const isActive = (link) => activeLinks().has(link);

function writeDevice(line) {
  for (const l of activeLinks()) l.write(line);
}
function syncLink(link) {
  link.write("unpet *");
  for (const p of pets.values()) link.write(petLine(p));
}
function relinked() {  // after links change, make every active link current
  for (const l of activeLinks()) if (l.syncedAs !== "active") { l.syncedAs = "active"; syncLink(l); }
  for (const l of links) if (!activeLinks().has(l)) l.syncedAs = "standby";
}

function lanAddress() {
  for (const addrs of Object.values(os.networkInterfaces())) {
    for (const a of addrs ?? []) if (a.family === "IPv4" && !a.internal) return a.address;
  }
  return "0.0.0.0";
}
const hostName = () => os.hostname().replace(/\.local$/, "").replace(/[^A-Za-z0-9-]/g, "-").slice(0, 40) || "mac";

function pickStyle(id) {
  const used = new Set([...pets.values()].map((p) => p.style));
  let h = 0;
  for (const c of id) h = (h * 31 + c.charCodeAt(0)) >>> 0;
  for (let i = 0; i < STYLE_COUNT; i++) {
    const s = (h + i) % STYLE_COUNT;
    if (!used.has(s)) return s;
  }
  return h % STYLE_COUNT;
}

function petLine(p) { return `pet ${p.id} ${p.state} ${p.style} ${clean(p.name, 24)}|${clean(p.detail, 40)}`; }

function broadcast(msg, set = clients) {
  const text = JSON.stringify(msg) + "\n";
  for (const c of set) c.write(text);
}

function statusMsg() {
  const devices = loadDevices();
  return {
    t: "status",
    port: usb?.path ?? null,
    deviceReady: [...links].some((l) => l.ready),
    links: [...links].map((l) => ({ kind: l.kind, device: l.deviceId ?? null, active: isActive(l), authed: !!l.authed })),
    wifi: WIFI_ENABLED ? { dialing: [...dialing], devices: Object.fromEntries(Object.entries(devices).map(([k, v]) => [k, v.ip ?? null])) } : null,
    paired: Object.keys(devices),
    pets: [...pets.values()].map(({ owner, ...p }) => p),
  };
}

// USB: whenever the board is attached, pair it (if needed) and keep its hub address current.
function onUsbHello(link, fields) {
  link.deviceId = fields.id;
  link.authed = true;  // physical attachment is the trust anchor
  const devices = loadDevices();
  let rec = devices[fields.id];
  const needPair = !rec || fields.paired !== "1" || fields.key !== keyPrint(rec.token);
  if (needPair) {
    rec = { token: rec?.token ?? crypto.randomBytes(32).toString("hex"), pairedAt: new Date().toISOString() };
    link.write(`pair ${rec.token} ${hostName()} ${lanAddress()} ${DEVICE_PORT}`);
    log("paired device", fields.id, "over USB");
  }
  if (fields.ip && fields.ip !== "-") rec.ip = fields.ip;  // where to dial it over Wi-Fi
  devices[fields.id] = rec;
  saveDevices(devices);
  link.info = fields;
  relinked();
}

function parseFields(line) {
  const f = {};
  for (const part of line.split(" ").slice(1)) { const i = part.indexOf("="); if (i > 0) f[part.slice(0, i)] = part.slice(i + 1); }
  return f;
}

function onDeviceLine(link, line) {
  if (monitors.size) broadcast({ t: "device", link: link.kind, line }, monitors);
  if (line.startsWith("GB_")) link.ready = true;
  if (line.startsWith("GB_HELLO ")) {
    const f = parseFields(line);
    if (link.kind === "usb") onUsbHello(link, f);
    else link.info = f;
    return;
  }
  if (link.kind === "usb" && (line.startsWith("GB_READY") || line.startsWith("GB_PAIRED"))) {
    if (line.startsWith("GB_READY")) { link.syncedAs = null; link.write("hello"); }
    return;
  }
  if (!isActive(link)) return;  // standby link: ignore duplicate device events
  if (line.startsWith("GB_FOCUS ")) broadcast({ t: "focus", id: line.slice(9).trim() });
  else if (line.startsWith("GB_ACK ")) broadcast({ t: "ack", id: line.slice(7).trim() });
}

async function findPort() {
  if (process.env.PI_PET_PORT === "none") return null;  // tests: no board
  if (process.env.PI_PET_PORT) return process.env.PI_PET_PORT;
  const ports = await SerialPort.list();
  const esp = ports.find((p) => (p.vendorId ?? "").toLowerCase() === "303a");
  return esp ? esp.path.replace("/dev/tty.", "/dev/cu.") : null;
}

async function connectSerial() {
  if (usb || Date.now() < releasedUntil || Date.now() < pausedUntil) return;
  const found = await findPort().catch(() => null);
  if (!found || usb) return;
  const p = new SerialPort({ path: found, baudRate: 115200, autoOpen: false });
  p.open((err) => {
    if (err) { log("serial open failed", found, err.message); return; }
    const link = { kind: "usb", write: (line) => p.isOpen && p.write(line + "\n"), close: () => p.close() };
    usb = { link, port: p, path: found, rx: "" };
    links.add(link);
    log("serial open", found);
    // Opening the port may reset the board (it then announces GB_READY); ask who it is either way.
    setTimeout(() => { if (usb?.port === p) link.write("hello"); }, 400);
    setTimeout(() => { if (usb?.port === p && !link.deviceId) { link.syncedAs = null; relinked(); } }, 1500);
  });
  p.on("data", (buf) => {
    if (!usb || usb.port !== p) return;
    usb.rx += buf.toString("latin1");
    let i;
    while ((i = usb.rx.indexOf("\n")) >= 0) {
      const line = usb.rx.slice(0, i).replace(/\r$/, "");
      usb.rx = usb.rx.slice(i + 1);
      if (line) onDeviceLine(usb.link, line);
    }
    if (usb.rx.length > 8192) usb.rx = usb.rx.slice(-1024);  // binary screenshot or noise
  });
  p.on("close", () => {
    log("serial closed");
    if (usb?.port === p) { links.delete(usb.link); usb = null; relinked(); }
  });
  p.on("error", (e) => log("serial error", e.message));
}

// Wi-Fi: the hub dials; the board greets. HELLO id nonceD -> CHALLENGE nonceH macH -> AUTH macD -> OK.
const dialing = new Set();
function dialDevices() {
  if (!WIFI_ENABLED || Date.now() < pausedUntil) return;
  const devices = loadDevices();
  for (const [id, rec] of Object.entries(devices)) {
    if (dialing.has(id) || [...links].some((l) => l.kind === "wifi" && l.deviceId === id)) continue;
    rec.tries = (rec.tries ?? 0);
    const useName = !rec.ip || (dialTries.get(id) ?? 0) % 2 === 1;  // alternate last IP and .local name
    dialTries.set(id, (dialTries.get(id) ?? 0) + 1);
    const host = useName ? `pi-pet-${id.slice(8)}.local` : rec.ip;
    dialing.add(id);
    const sock = net.connect({ host, port: DEVICE_PORT });
    sock.setTimeout(4000, () => sock.destroy());
    sock.on("connect", () => { sock.setTimeout(0); dialTries.set(id, 0); handleDeviceSocket(sock, id); });
    sock.on("close", () => dialing.delete(id));
    sock.on("error", () => {});
  }
}
const dialTries = new Map();

function handleDeviceSocket(sock, expectId) {
  sock.setEncoding("latin1");
  sock.setNoDelay(true);
  const link = { kind: "wifi", write: (line) => !sock.destroyed && sock.write(line + "\n"), close: () => sock.destroy() };
  let stage = "hello", nonceD = "", nonceH = "", token = "", buf = "";
  const timer = setTimeout(() => { if (!link.authed) sock.destroy(); }, 8000);
  sock.on("data", (chunk) => {
    buf += chunk;
    if (buf.length > 16384) { sock.destroy(); return; }
    let i;
    while ((i = buf.indexOf("\n")) >= 0) {
      const line = buf.slice(0, i).replace(/\r$/, "");
      buf = buf.slice(i + 1);
      if (stage === "hello") {
        const m = line.match(/^HELLO ([0-9a-f]{12}) ([0-9a-f]{16})$/);
        const rec = m && m[1] === expectId && loadDevices()[m[1]];
        if (!rec) { sock.end("DENY unpaired\n"); log("wifi peer refused", m?.[1] ?? line.slice(0, 20)); return; }
        link.deviceId = m[1]; nonceD = m[2]; token = rec.token;
        nonceH = crypto.randomBytes(16).toString("hex");
        sock.write(`CHALLENGE ${nonceH} ${hmac(token, `hub:${nonceD}:${nonceH}`)}\n`);
        stage = "auth";
      } else if (stage === "auth") {
        const m = line.match(/^AUTH ([0-9a-f]{64})$/);
        const want = hmac(token, `dev:${nonceH}:${nonceD}`);
        if (!m || !crypto.timingSafeEqual(Buffer.from(m[1]), Buffer.from(want))) {
          sock.end("DENY auth\n"); log("wifi device failed auth", link.deviceId); return;
        }
        sock.write("OK\n");
        link.authed = true;
        clearTimeout(timer);
        links.add(link);
        stage = "open";
        log("wifi link up", link.deviceId, sock.remoteAddress);
        link.syncedAs = null;
        relinked();
        link.write("hello");
      } else if (line) {
        onDeviceLine(link, line);
      }
    }
  });
  const drop = () => {
    clearTimeout(timer);
    if (links.delete(link)) { log("wifi link down", link.deviceId); relinked(); }
  };
  sock.on("close", drop);
  sock.on("error", () => {});
}

// /pet wifi: read the network password from the login keychain (macOS asks for approval) and
// push it to the board over USB into the shared ESPtember Wi-Fi slot. Never logged.
function run(cmd, args) {
  return new Promise((resolve) => execFile(cmd, args, { timeout: 60_000 }, (err, stdout) => resolve(err ? null : stdout)));
}
async function provisionWifi(ssidArg) {
  if (!usb?.link) return { ok: false, message: "connect the Stopwatch over USB first" };
  let ssid = ssidArg;
  if (!ssid) {
    const out = await run("networksetup", ["-listpreferredwirelessnetworks", "en0"]);
    ssid = out?.split("\n").slice(1).map((s) => s.trim()).find(Boolean);
  }
  if (!ssid) return { ok: false, message: "no Wi-Fi network name; use /pet wifi <ssid>" };
  const pass = await run("security", ["find-generic-password", "-D", "AirPort network password", "-a", ssid, "-w"]);
  if (pass == null) return { ok: false, message: `no keychain password for "${ssid}" (or access declined)` };
  const b64 = (s) => Buffer.from(s, "utf8").toString("base64");
  usb.link.write(`wifi ${b64(ssid)} ${b64(pass.replace(/\n$/, ""))}`);
  log("pushed Wi-Fi credentials over USB for", JSON.stringify(ssid));
  return { ok: true, message: `sent Wi-Fi "${ssid}" to the Stopwatch over USB` };
}

function forgetDevices() {
  for (const l of links) if (l.kind === "usb") l.write("forget");
  saveDevices({});
  for (const l of [...links]) if (l.kind === "wifi") l.close();
  return { ok: true, message: "forgot pairing; plug in over USB to pair again" };
}

function upsert(id, patch, owner) {
  const prev = pets.get(id);
  if (!prev && pets.size >= MAX_PETS) return;
  const next = prev ? { ...prev, ...patch } : { id, name: "pi", state: "idle", detail: "", style: pickStyle(id), owner, ...patch };
  if (!STATES.has(next.state)) next.state = "idle";
  next.name = clean(next.name, 24) || "pi";  // store exactly what the device will show
  next.detail = clean(next.detail, 40);
  pets.set(id, next);
  if (!prev || petLine(prev) !== petLine(next)) writeDevice(petLine(next));
}

function remove(id) {
  if (!pets.delete(id)) return;
  writeDevice(`unpet ${id}`);
}

function handleClient(sock) {
  clients.add(sock);
  lastClientAt = Date.now();
  const owned = new Set();
  let buf = "";
  sock.setEncoding("utf8");
  const reply = (msg) => sock.write(JSON.stringify(msg) + "\n");
  sock.on("data", (chunk) => {
    buf += chunk;
    let i;
    while ((i = buf.indexOf("\n")) >= 0) {
      const text = buf.slice(0, i);
      buf = buf.slice(i + 1);
      let msg;
      try { msg = JSON.parse(text); } catch { continue; }
      const id = msg.id ? shortId(msg.id) : null;
      switch (msg.t) {
        case "hello": owned.add(id); upsert(id, { name: msg.name || path.basename(msg.cwd || "") || "pi" }, sock); break;
        case "state": owned.add(id); upsert(id, { state: msg.state, detail: msg.detail ?? "" }, sock); break;
        case "bye": owned.delete(id); remove(id); break;
        case "raw": writeDevice(clean(msg.line, 120)); break;
        case "monitor": monitors.add(sock); break;
        case "status": reply(statusMsg()); break;
        case "wifi": provisionWifi(msg.ssid ? clean(msg.ssid, 32) : "").then((r) => reply({ t: "result", ...r })); break;
        case "forget": reply({ t: "result", ...forgetDevices() }); break;
        case "pause": {  // tests: hands off the board completely for `seconds` (default 300)
          pausedUntil = Date.now() + 1000 * Math.min(Number(msg.seconds) || 300, 3600);
          log("paused for", Math.round((pausedUntil - Date.now()) / 1000), "s");
          usb?.port.close();
          for (const l of [...links]) if (l.kind === "wifi") l.close();
          reply({ t: "result", ok: true, message: "paused" });
          break;
        }
        case "resume": pausedUntil = 0; log("resumed"); reply({ t: "result", ok: true, message: "resumed" }); break;
        case "release":  // let esptool have the port for a while (Wi-Fi keeps the pets live)
          releasedUntil = Date.now() + 30_000;
          log("serial released for 30 s");
          usb?.port.close();
          break;
      }
    }
    if (buf.length > 65536) sock.destroy();
  });
  const drop = () => {
    clients.delete(sock); monitors.delete(sock);
    for (const id of owned) remove(id);  // a crashed pi session takes its pet with it
    lastClientAt = Date.now();
  };
  sock.on("close", drop);
  sock.on("error", () => {});
}

function main() {
  fs.mkdirSync(HUB_DIR, { recursive: true });
  // A stale socket from a dead hub blocks listen(); a live hub answers.
  const probe = net.createConnection(SOCKET_PATH);
  probe.on("connect", () => { console.error("pi-pet hub already running"); process.exit(0); });
  probe.on("error", () => {
    try { fs.unlinkSync(SOCKET_PATH); } catch {}
    const server = net.createServer(handleClient);
    server.listen(SOCKET_PATH, () => log("hub listening", SOCKET_PATH, "pid", process.pid));
    connectSerial();
    setInterval(connectSerial, 2000);
    setInterval(dialDevices, 3000);
    // Heartbeat: the board clears a stale roster (screensaver) after 20 s without hub traffic.
    setInterval(() => writeDevice("ping"), 5000);
    setInterval(() => {
      if (!clients.size && Date.now() - lastClientAt > IDLE_EXIT_MS) { log("idle; exiting"); bye(); }
    }, 10_000);
    const bye = () => {
      try { writeDevice("unpet *"); } catch {}
      try { fs.unlinkSync(SOCKET_PATH); } catch {}
      process.exit(0);
    };
    process.on("SIGTERM", bye);
    process.on("SIGINT", bye);
  });
}

main();
