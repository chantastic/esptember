import assert from 'node:assert/strict';
import { readFileSync, existsSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
const read = path => readFileSync(path, 'utf8');
const published = readdirSync('days').filter(slug => existsSync(`days/${slug}/README.md`));
const home = read('dist/index.html'), index = read('dist/llms.txt');
for (const slug of published) {
  const base = `dist/day/${slug}`;
  const guide = read(`${base}/index.html`), story = read(`${base}/story/index.html`);
  const source = read(`days/${slug}/README.md`);
  const body = source.replace(/^---\r?\n[\s\S]*?\r?\n---/, '').trim();
  assert(read(`${base}/guide.md`).trim().endsWith(body), `${slug}: Markdown diverged from README`);
  assert(home.includes(`/day/${slug}/`) && index.includes(`/day/${slug}/guide.md`), `${slug}: missing from index`);
  for (const html of [guide, story]) {
    for (const [, anchor] of html.matchAll(/data-heading="([^"]+)"/g)) {
      assert(html.includes(`id="${anchor}"`), `${slug}: broken heading link ${anchor}`);
    }
    assert(html.includes('Plain Markdown'), `${slug}: missing Markdown link`);
  }
  assert(guide.includes('Recorded evidence'), `${slug}: missing verification scope`);
  assert(story.includes('What we learned'), `${slug}: missing story conclusion`);
  const manifest = JSON.parse(read(`${base}/manifest.json`));
  const part = manifest.builds[0].parts[0];
  assert.equal(part.offset, 0);
  assert.equal(part.path, `/firmware/${slug}.bin`);
  assert(readFileSync(join('dist', part.path)).equals(readFileSync(`public/firmware/${slug}.bin`)), `${slug}: firmware mismatch`);
  // Excerpts may skip intervening code, but every executable line must exist in the source.
  const firmware = read(`days/${slug}/firmware/main/main.c`);
  for (const [, block] of body.matchAll(/```c\n([\s\S]*?)\n```/g)) {
    for (const line of block.split('\n').map(line => line.trim()).filter(line => line && !line.startsWith('//'))) {
      assert(firmware.includes(line), `${slug}: excerpt drift: ${line}`);
    }
  }
}
for (const slug of readdirSync('days').filter(slug => !published.includes(slug))) {
  assert(!existsSync(`dist/day/${slug}`), `${slug}: draft route published`);
  assert(!existsSync(`dist/firmware/${slug}.bin`), `${slug}: draft firmware published`);
}
console.log(`Verified ${published.length} guide/story pairs, Markdown parity, heading anchors, excerpts, firmware manifests, and draft exclusion.`);
