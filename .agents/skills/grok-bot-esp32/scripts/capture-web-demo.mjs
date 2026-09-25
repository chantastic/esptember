#!/usr/bin/env node
// Record the live "Avatar motion system" demo on x.ai/news/designing-grok-bot, frame by frame.
//
//   npm i puppeteer-core@23        (in a scratch directory; uses the installed Google Chrome)
//   node capture-web-demo.mjs <outDir> [seconds=34] [svgPx=420]
//
// Writes <outDir>/fNNNNN.jpg (full-viewport CDP screencast, ~55-60 fps), log.tsv
// (frame, ms since start, svg data-state) and box.json (the enlarged avatar's viewport box).
// The page cycles Idle, Working, Waiting(bored), Blocked(alerting), Thinking, Done(celebrate).
// Direct media downloads from media.x.ai return 403; capture in-page instead.
import puppeteer from "puppeteer-core";
import fs from "node:fs";

const [outDir = "motion", secs = "34", svgPx = "420"] = process.argv.slice(2);
const CHROME = process.env.CHROME || "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
fs.mkdirSync(outDir, { recursive: true });
const browser = await puppeteer.launch({ executablePath: CHROME, headless: "new", args: ["--window-size=1200,1000"] });
const page = await browser.newPage();
await page.setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36");
await page.setViewport({ width: 1200, height: 1000, deviceScaleFactor: 1 });
await page.goto("https://x.ai/news/designing-grok-bot", { waitUntil: "domcontentloaded", timeout: 60000 });
await new Promise((r) => setTimeout(r, 5000));
const found = await page.evaluate((px) => {
  const labels = ["Idle", "Working", "Waiting", "Blocked", "Thinking", "Done"];
  const leaf = [...document.querySelectorAll("*")].find((e) => e.children.length === 0 && labels.includes(e.textContent.trim()));
  let box = leaf;
  while (box && !labels.every((l) => box.textContent.includes(l))) box = box.parentElement;
  while (box && !box.querySelector("svg[data-state]")) box = box.parentElement;
  if (!box) return null;
  box.id = "motion-demo";
  const svg = box.querySelector("svg[data-state]");
  svg.style.width = svg.style.height = px + "px";
  for (let pe = svg.parentElement, k = 0; pe && k < 4; pe = pe.parentElement, k++) pe.style.overflow = "visible";
  document.querySelectorAll('[class*="cookie"], [id*="cookie"]').forEach((e) => e.remove());
  svg.scrollIntoView({ block: "center" });
  return svg.dataset.state;
}, svgPx);
if (!found) { console.error("motion demo not found (page layout changed?)"); process.exit(1); }
await new Promise((r) => setTimeout(r, 1000));
const bb = await (await page.$("#motion-demo svg[data-state]")).boundingBox();
fs.writeFileSync(`${outDir}/box.json`, JSON.stringify(bb));
const cdp = await page.createCDPSession();
let n = 0; const t0 = Date.now(); const log = [];
cdp.on("Page.screencastFrame", async (f) => {
  const t = Date.now() - t0;
  fs.writeFileSync(`${outDir}/f${String(n).padStart(5, "0")}.jpg`, Buffer.from(f.data, "base64"));
  const st = await page.evaluate(() => document.querySelector("#motion-demo svg[data-state]")?.dataset.state ?? "").catch(() => "");
  log.push(`${n}\t${t}\t${st}`); n++;
  await cdp.send("Page.screencastFrameAck", { sessionId: f.sessionId }).catch(() => {});
});
await cdp.send("Page.startScreencast", { format: "jpeg", quality: 90, everyNthFrame: 1 });
await new Promise((r) => setTimeout(r, +secs * 1000));
await cdp.send("Page.stopScreencast");
fs.writeFileSync(`${outDir}/log.tsv`, log.join("\n"));
console.log(`frames ${n} (${(n / +secs).toFixed(1)} fps) -> ${outDir}`);
await browser.close();
