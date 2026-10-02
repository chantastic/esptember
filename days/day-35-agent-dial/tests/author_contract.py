#!/usr/bin/env python3
"""Author tests/contract.json from compact scenarios.

Each scenario lists actions with the claims that matter. Complete expectations (every changed
field) come from reference_model.py, and any claim that disagrees with the model aborts. Edit
scenarios here, never contract.json by hand, then run tests/run.sh.
"""
import json, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from reference_model import reduce, action_object, SCREENS

state = {}
for s in "abc":
  state[f"{s}_state"] = {"type":"enum","default":"none","values":["none","idle","working"]}
  state[f"{s}_asking"] = {"type":"bool","default":False}
  state[f"{s}_unread"] = {"type":"enum","default":"none","values":["none","done","question"]}
  state[f"{s}_stop"] = {"type":"bool","default":False}
  state[f"{s}_prompt"] = {"type":"bool","default":False}
state |= {
  "connected": {"type":"bool","default":False},
  "screen": {"type":"enum","default":"overview","values":SCREENS},
  "focus": {"type":"enum","default":"none","values":["none","a","b","c"]},
  "voice": {"type":"enum","default":"off","values":["off","recording","routing"]},
  "voice_target": {"type":"enum","default":"none","values":["none","a","b","c","routed"]},
  "awake": {"type":"bool","default":True},
  "badge": {"type":"int","default":0,"min":0,"max":3},
  "emit": {"type":"string","default":"","max_length":24},
  "sound": {"type":"enum","default":"none","values":["none","beep"]},
  "toast": {"type":"string","default":"","max_length":40},
}
PI = lambda i: {"type":"agent","id":i,"stop":True,"prompt":True}        # pi: stop + prompt
CC = lambda i: {"type":"agent","id":i,"stop":False,"prompt":False}      # Claude Code: approve only
A = lambda t, i, **kw: {"type":t,"id":i, **kw}
UP = [("welcome", {"connected":True}), (PI("a"), {"focus":"a","a_state":"idle","emit":""})]

scen = [
 ("the first session is focused without an echo", {}, [
   ("welcome", {"connected":True}),
   (PI("a"), {"focus":"a","a_state":"idle","screen":"overview","emit":""}),
   (CC("b"), {"focus":"a","b_state":"idle"})]),
 ("pushers leave the overview for the focused tile and page with wrap", {}, UP + [
   (CC("b"), {}), (PI("c"), {}),
   ("right", {"screen":"agent","focus":"a","emit":"focus a"}),
   ("right", {"focus":"b","emit":"focus b"}),
   ("right", {"focus":"c","emit":"focus c"}),
   ("right", {"focus":"a","emit":"focus a"}),
   ("left", {"focus":"c","emit":"focus c"})]),
 ("bottom-edge swipe toggles tile and overview", {}, UP + [
   ("swipe_up_bottom", {"screen":"agent","emit":"focus a"}),
   ("swipe_up_bottom", {"screen":"overview","emit":""})]),
 ("a working tile shows the turn and returns to idle silently", {}, UP + [
   (A("turn_started","a"), {"a_state":"working","sound":"none"}),
   (A("turn_done","a"), {"a_state":"idle","sound":"none","badge":0})]),
 ("a turn error toasts without a drawer row", {}, UP + [
   (A("turn_started","a"), {}),
   (A("turn_error","a"), {"a_state":"idle","toast":"turn error","badge":0})]),
 ("a summary beeps and files a row unless you are looking", {}, UP + [
   (CC("b"), {}),
   ("right", {"screen":"agent","focus":"a"}),
   (A("turn_started","b"), {}),
   (A("summary","b",quiet=False), {"b_state":"idle","b_unread":"done","badge":1,"sound":"beep"}),
   (A("summary","a",quiet=False), {"a_unread":"none","badge":1,"sound":"beep"}),
   ("right", {"focus":"b","b_unread":"none","badge":0,"emit":"focus b"})]),
 ("a quiet summary still beeps but files no row", {}, UP + [
   (A("summary","a",quiet=True), {"sound":"beep","a_unread":"none","badge":0})]),
 ("a summary wakes a sleeping dial onto the drawer", {}, UP + [
   ("idle_5m", {"awake":False}),
   (A("summary","a",quiet=False), {"awake":True,"screen":"drawer","a_unread":"done","badge":1,"sound":"beep"}),
   ({"type":"open_row","id":"a"}, {"screen":"agent","focus":"a","a_unread":"none","badge":0,"emit":"agent.open a"})]),
 ("a working tile keeps the dial awake", {}, UP + [
   ("right", {"screen":"agent"}),
   (A("turn_started","a"), {}),
   ("idle_5m", {"awake":True}),
   (A("turn_done","a"), {}),
   ("idle_5m", {"awake":False})]),
 ("the waking input is swallowed", {}, UP + [
   ("idle_5m", {"awake":False}),
   ("right", {"awake":True,"screen":"overview","emit":""}),
   ("right", {"screen":"agent","emit":"focus a"})]),
 ("an approval opens, is answered, and returns to the tile", {}, UP + [
   (CC("b"), {}),
   (A("turn_started","b"), {}),
   (A("question","b"), {"b_asking":True,"b_unread":"question","badge":1,"screen":"question","focus":"b",
                        "sound":"beep","emit":"agent.open b"}),
   ("answer", {"emit":"answer b","b_asking":False,"b_unread":"none","badge":0,"screen":"agent"})]),
 ("back dismisses a question but keeps its row until answered", {}, UP + [
   (A("question","a"), {"screen":"question"}),
   ("back", {"screen":"agent","a_asking":True,"a_unread":"question","badge":1}),
   ("swipe_down_top", {"screen":"drawer"}),
   ({"type":"open_row","id":"a"}, {"screen":"question","a_unread":"question","emit":"agent.open a"}),
   ("answer", {"screen":"agent","badge":0})]),
 ("a second question waits in the drawer", {}, UP + [
   (CC("b"), {}),
   (A("question","a"), {"screen":"question","focus":"a"}),
   (A("question","b"), {"screen":"question","focus":"a","b_unread":"question","badge":2,"emit":"agent.open b"}),
   ("answer", {"screen":"agent","badge":1}),
   ("swipe_down_top", {"screen":"drawer"}),
   ({"type":"open_row","id":"b"}, {"screen":"question","focus":"b"})]),
 ("question.close answered elsewhere returns to the tile", {}, UP + [
   (A("question","a"), {"screen":"question"}),
   (A("question_close","a"), {"screen":"agent","a_asking":False,"a_unread":"none","badge":0})]),
 ("a done row never replaces a question row", {}, UP + [
   (A("question","a"), {}),
   ("back", {}),
   ("swipe_up_bottom", {"screen":"overview"}),
   (A("summary","a",quiet=False), {"a_unread":"question","badge":1,"sound":"beep"})]),
 ("stop asks first and needs the stop capability", {}, UP + [
   (CC("b"), {}),
   ("right", {"screen":"agent","focus":"a"}),
   (A("turn_started","a"), {}),
   ("back", {"screen":"confirm"}),
   ("confirm_no", {"screen":"agent","a_state":"working"}),
   ("back", {"screen":"confirm"}),
   ("confirm_yes", {"emit":"turn.stop a","a_state":"idle","screen":"agent"}),
   ("right", {"focus":"b"}),
   (A("turn_started","b"), {}),
   ("back", {"screen":"agent","toast":"Can't stop this from here"})]),
 ("voice records to the focused tile and routes after release", {}, UP + [
   ("right", {"screen":"agent"}),
   ("enter", {"voice":"recording","voice_target":"a","screen":"voice","emit":"voice.begin a"}),
   ("enter", {"voice":"routing","emit":"voice.end"}),
   (A("transcript","a"), {"voice":"routing","focus":"a"}),
   (A("turn_started","a"), {"voice":"off","voice_target":"none","screen":"agent","a_state":"working"})]),
 ("a tap anywhere stops a recording", {}, UP + [
   ("right", {}), ("enter", {"voice":"recording"}),
   ("tap", {"voice":"routing","emit":"voice.end"})]),
 ("back aborts a recording", {}, UP + [
   ("right", {}), ("enter", {"voice":"recording"}),
   ("back", {"voice":"off","voice_target":"none","screen":"agent","emit":"voice.abort"})]),
 ("a routed take starts from the overview", {}, UP + [
   ("enter", {"voice":"recording","voice_target":"routed","screen":"voice","emit":"voice.begin"}),
   ("enter", {"voice":"routing"}),
   ("voice_error", {"voice":"off","voice_target":"none","screen":"agent","toast":"voice error"})]),
 ("routing gives up after a minute", {}, UP + [
   ("enter", {}), ("enter", {"voice":"routing"}),
   ("advance_60s", {"voice":"off","screen":"agent"})]),
 ("voice needs the prompt capability and a connection", {}, UP + [
   (CC("b"), {}),
   ("right", {"screen":"agent"}), ("right", {"focus":"b"}),
   ("enter", {"voice":"off","toast":"Can't send to this harness"}),
   ("silence_15s", {"connected":False,"focus":"none","screen":"overview","b_state":"none","a_state":"none"}),
   ("enter", {"toast":"Not connected","voice":"off"})]),
 ("no beep and no interruption while the mic runs", {}, UP + [
   (CC("b"), {}),
   ("right", {}), ("enter", {"voice":"recording"}),
   (A("question","b"), {"sound":"none","screen":"voice","b_unread":"question","badge":1,"emit":"agent.open b"}),
   (A("summary","a",quiet=False), {"sound":"none","a_unread":"done","badge":2})]),
 ("silence clears everything, including a recording, without sending", {}, UP + [
   (A("question","a"), {}),
   ("back", {}),
   ("enter", {"voice":"off"}),
   ("silence_15s", {"connected":False,"a_state":"none","a_asking":False,"a_unread":"none","a_stop":False,
                    "a_prompt":False,"badge":0,"focus":"none","screen":"overview","emit":""})]),
 ("a removed session moves focus and closes its question", {}, UP + [
   (CC("b"), {}),
   ("right", {}),
   (A("question","a"), {"screen":"question"}),
   ({"type":"gone","id":"a"}, {"a_state":"none","a_asking":False,"a_unread":"none","badge":0,"focus":"b","screen":"agent"}),
   ({"type":"gone","id":"b"}, {"focus":"none","b_state":"none"})]),
 ("a recording to a removed session is dropped", {}, UP + [
   (PI("b"), {}),
   ("right", {}), ("right", {"focus":"b"}), ("enter", {"voice_target":"b"}),
   ({"type":"gone","id":"b"}, {"voice":"off","voice_target":"none","screen":"agent","focus":"a"})]),
 ("reports for unknown sessions are ignored", {}, UP + [
   (A("summary","c",quiet=False), {"sound":"none","badge":0}),
   (A("question","c"), {"screen":"overview","emit":""})]),
 ("settings open from the overview and close with back", {}, UP + [
   ("open_settings", {"screen":"settings"}),
   ("right", {"screen":"settings","emit":""}),
   ("back", {"screen":"overview"})]),
 ("the drawer opens from the top and closes with back", {}, UP + [
   ("swipe_down_top", {"screen":"drawer"}),
   ("left", {"screen":"drawer","emit":""}),
   ("back", {"screen":"agent"})]),
 ("removing the focused session moves to the next one", {}, UP + [
   (CC("b"), {}), (PI("c"), {}),
   ("right", {"screen":"agent","focus":"a"}), ("right", {"focus":"b"}),
   ({"type":"gone","id":"b"}, {"focus":"c","b_state":"none"}),
   ({"type":"gone","id":"c"}, {"focus":"a"})]),
 ("capabilities update in place", {}, UP + [
   (CC("a"), {"a_stop":False,"a_prompt":False,"a_state":"idle"}),
   (PI("a"), {"a_stop":True,"a_prompt":True})]),
]
defaults = {k:v["default"] for k,v in state.items()}
out = []
for name, given, steps in scen:
  st = defaults | given
  rows = []
  for action, claim in steps:
    model = reduce(st, action_object(action))
    for k, v in claim.items():
      if model[k] != v: sys.exit(f"CLAIM MISMATCH {name}: {action} {k} claimed {v!r} model {model[k]!r}")
    expect = {k: model[k] for k in model if model[k] != st[k]} | claim
    rows.append({"action": action, "expect": expect})
    st = model
  sc = {"name": name, "steps": rows}
  if given: sc["given"] = given
  out.append(sc)
contract = {
 "schema": 1,
 "lesson": "day-35-agent-dial",
 "title": "Agent Dial",
 "hardware": {"board":"m5stack-stopwatch","width":468,"height":466,"offset_x":6,"offset_y":0,"rotation":0,"touch_map":"espt-touch/record"},
 "controls": {"profile":"c25k","left_button":"BtnA","right_button":"BtnB",
   "left":"previous_or_decrease","right":"next_or_increase","enter":"short_both","back":"hold_both_600ms",
   "rationale":"Left/right replace upstream's chevrons and horizontal swipes. Enter is the tile's Voice button (stop-and-send while recording). Back is upstream's BOOT press: dismiss, Cancel task?, abort, or close."},
 "state": state,
 "scenarios": out,
 "layout": {"safe_radius":226,"focus_inset":3,"controls":[
   {"id":"overview_inbox","bounds":[79,308,64,64]},
   {"id":"overview_voice","bounds":[190,346,88,88]},
   {"id":"overview_settings","bounds":[325,308,64,64]},
   {"id":"tile_name","bounds":[74,119,320,48]},
   {"id":"recap_card","bounds":[42,188,384,100]},
   {"id":"tile_voice","bounds":[194,353,80,80]},
   {"id":"tile_previous","bounds":[80,322,80,80]},
   {"id":"tile_next","bounds":[308,322,80,80]},
   {"id":"connection_badge","bounds":[164,22,140,24]},
   {"id":"confirm_yes","bounds":[94,250,280,64]},
   {"id":"confirm_back","bounds":[94,326,280,64]}]},
 "visual": {"kind":"card","eyebrow":"AGENT DIAL","title":"esptember","lines":["Cooking… 34s","$ npm test"],"accent":"#00FF2F",
   "scenario":"stop asks first and needs the stop capability","evidence":"reference"},
}
p = HERE / "contract.json"
open(p,"w").write(json.dumps(contract, indent=2, ensure_ascii=False) + "\n")
print("wrote", p, sum(len(s["steps"]) for s in out), "steps")
