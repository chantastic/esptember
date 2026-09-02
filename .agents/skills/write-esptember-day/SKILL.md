---
name: write-esptember-day
description: Write the README for an ESPtember day — the writeup that doubles as the day's page on esptember.com and its directory on GitHub. Use when a day's firmware is finished and verified on hardware, or when revising an existing day post. Covers structure, voice, fact-checking against the source, and the deploy loop.
role: transformation
---

# Write ESPtember Day

Write one day's README.md for the ESPtember project (~/Developer/esptember).
The README is the single source of truth: Astro renders it at `esptember.com/day/<slug>/`, GitHub renders it in the repo, and its frontmatter drives the site's index, video embed, firmware download, and ESP Web Tools manifest.

Consult `consult-chan-writing-style` before drafting. This skill adds the post-shaped specifics.

## Inputs

| Input | Required | Discovery | Description |
|-------|----------|-----------|-------------|
| day_dir | yes | ask | Path to `days/day-NN-slug/` |
| firmware verified | yes | ask | Confirmation the final build ran on hardware (soak-tested if stability was ever in question) |
| story | yes | infer from session; confirm | What actually happened: what was tried, what broke, what was learned. The debugging session IS the material |
| toolchain | yes | discover: firmware dir contents | e.g. "ESP-IDF v5.5 + Waveshare BSP (LVGL)" — noted in frontmatter every day |
| video | no | ask | YouTube URL; added after recording, not before |

## Outputs

| Output | Type | Description |
|--------|------|-------------|
| `days/day-NN-slug/README.md` | source | The post. Frontmatter + writeup |
| NOTES.md additions | source | Lesson ideas, open questions, and retest items the day generated |
| deployed site | artifact | Via the deploy loop (below) |

## Frontmatter contract

```yaml
---
day: 1                                      # integer, drives sort
title: Hello World
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-NN-slug.bin         # omit if no flashable artifact
video: https://youtu.be/ID                  # omit until published
---
```

## Structure

**Principle:** open with the assignment, close with the idea.
Logistics live in the middle.

1. **The assignment.** One or two beats. What today asks for.
2. **The code.** One block, the heart of the day. Comments carry the narrative — a comment that explains *why a line survived* beats a paragraph below the block.
3. **What went wrong** (if anything did). Name the failure the way the user experienced it, then investigate. One `###` per distinct bug.
4. **The proof.** If stability was ever in question, the verification gets its own beat ("Fifteen minutes, no flicker.").
5. **Flash it.** Download link → find your port → one command → payoff line.
6. **Build from source.** Two or three lines. The source renders on the page below the post; don't duplicate it.
7. **What we learned.** The close. Three or four morals, each earned *today* — no imported wisdom. End on the strongest sentence in the piece.

**Heuristic:** if nothing went wrong, cut sections 3–4 and let the post be short.
A day that just works is a short post, not a padded one.
The writing doesn't owe anyone a war story.

## Voice

Zinsser through Chan (see `consult-chan-writing-style` for the full lens):

- One sentence per line. Double returns between ideas.
- Active verbs. Few adjectives — an adjective survives only if it carries an argument ("the *whole* assignment").
- **This/but/then.** Each section gets one Hegelian turn: setup, reversal, landing. "The code above works. / Then, one to four minutes in, it doesn't."
- **Rhythm needs contrast.** All-staccato reads flat. Give each section one longer sentence that carries momentum, then snap back short.
- **There is no skeptic.** Never raise a doubt to knock it down, justify an unquestioned choice, or restate what the code already said. State it once, where it's strongest.
- Talk to a smart peer, having fun. The reader's code is not the suspect — tell them when the fault is below their code ("Neither is in your eight calls.").

## Fact discipline

**Principle:** every technical claim in the post was observed on hardware or verified in source.

- **Count things, then check the count.** "Six lines" shipped wrong once; the block had eight. Prefer robust units: "calls," not "lines" (comments stretch lines).
- **The README code block must match `main.c`.** The page renders the real source below the post; a drifted excerpt is a visible lie. Diff them before deploy.
- **Split failure modes honestly.** Don't fold two bugs into one symptom sentence if their symptoms differ.
- **Empirical claims carry their scope.** "Required: the one sent during init doesn't stick" was proven under conditions that later changed — when a fix might invalidate an earlier finding, add a retest item to NOTES.md rather than silently keeping (or softening) the claim.
- Register values, config names, and version numbers in prose must match the source files.

## Process

1. **Confirm the firmware is final and verified.** No post before the hardware behaves. If stability was ever in doubt, demand the soak result.
2. **Mine the session for the story.** The debugging transcript is the material: what was deleted, what broke, what the boot log said, which upstream issue matched. Write down the beats before drafting prose.
3. **Draft into the structure above.** Code block first — get the comments right, they do the heaviest lifting.
4. **Harvest side-material into NOTES.md:** future lesson ideas the day generated, open questions, retest items.
5. **Review pass** (separate from drafting):
   - Read for defensive tells: raise-and-dismiss, unprompted justification, restated points, pleading qualifiers ("genuinely," "actually").
   - Check every number against the code.
   - Check the closing line of each section — is it a beat or a shrug?
   - Confirm the piece ends on the idea, not logistics.
6. **Deploy loop:**
   ```sh
   # in days/day-NN-slug/firmware: idf.py build merge-bin (if firmware changed)
   cd ~/Developer/esptember
   pnpm run deploy       # collects bins, builds Astro, ships to Cloudflare
   git add -A && git commit && git push
   ```
7. **After the video publishes:** add `video:` to frontmatter, `pnpm run deploy`.

## Notes

- The site renders the full firmware source under the post ("Firmware source" section) and links the day's GitHub directory — the post excerpts, never mirrors.
- The prebuilt bin is the merged image flashed at `0x0`. The flash section is always one command; that simplicity is a feature the project already paid for.
- Frontmatter `toolchain:` exists because toolchains will vary across the month. Never omit it.
- Board reality: user's board is a **V2** (CO5300 panel, CST816-family touch, TCA9554-gated panel reset, AXP2101 PMU). Day 01's `pmu_init()` + `panel_reset_release()` are load-bearing on every future day until extracted into a shared component.
- Titles: the project style is `ESPtember` everywhere prose allows; URLs, filenames, and the domain stay lowercase.
