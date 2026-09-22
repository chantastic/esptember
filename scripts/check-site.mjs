import assert from 'node:assert/strict';
import { readFileSync, existsSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
const read = path => readFileSync(path, 'utf8');
const firmwareSources = directory => readdirSync(directory, { withFileTypes: true }).flatMap(entry => {
  const path = join(directory, entry.name);
  if (entry.isDirectory()) return ['build', 'managed_components', 'generated'].includes(entry.name) ? [] : firmwareSources(path);
  return /\.(c|h|cpp|hpp|ino)$/.test(entry.name) ? [path] : [];
});
const boards = JSON.parse(read('src/lib/boards.json'));
const published = readdirSync('days').filter(slug => existsSync(`days/${slug}/README.md`));
const home = read('dist/index.html'), index = read('dist/llms.txt');
for (const slug of published) {
  const base = `dist/day/${slug}`;
  const guide = read(`${base}/index.html`), story = read(`${base}/story/index.html`);
  const source = read(`days/${slug}/README.md`);
  const boardId = source.match(/^board: (.+)$/m)?.[1];
  const board = boards[boardId];
  assert(board, `${slug}: unknown or missing target board`);
  assert(guide.includes(board.model), `${slug}: target board missing from page`);
  assert(read(`${base}/guide.md`).includes(`Board: ${board.model}`), `${slug}: target board missing from Markdown`);
  assert(existsSync(join('dist', board.image)), `${slug}: board illustration missing`);
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
  assert.equal(manifest.builds[0].chipFamily, board.chip);
  const part = manifest.builds[0].parts[0];
  assert.equal(part.offset, 0);
  assert.equal(part.path, `/firmware/${slug}.bin`);
  assert(readFileSync(join('dist', part.path)).equals(readFileSync(`public/firmware/${slug}.bin`)), `${slug}: firmware mismatch`);
  // Excerpts may skip intervening code, but every executable line must exist in the source.
  const sourcePaths = firmwareSources(`days/${slug}/firmware`);
  assert(sourcePaths.length, `${slug}: no firmware source found`);
  const displayedSources = new Set([...guide.matchAll(/<summary\b[^>]*>\s*<code\b[^>]*>([^<]+)<\/code>\s*<\/summary>/g)].map(match => match[1]));
  for (const path of sourcePaths) {
    assert(displayedSources.has(path.split('/firmware/')[1]), `${slug}: source file missing from guide: ${path}`);
  }
  const firmware = sourcePaths.map(read).join('\n');
  for (const [, block] of body.matchAll(/```(?:c|cpp|c\+\+|arduino)\n([\s\S]*?)\n```/g)) {
    for (const line of block.split('\n').map(line => line.trim()).filter(line => line && !line.startsWith('//'))) {
      assert(firmware.includes(line), `${slug}: excerpt drift: ${line}`);
    }
  }
}
for (const slug of readdirSync('days').filter(slug => !published.includes(slug))) {
  assert(!existsSync(`dist/day/${slug}`), `${slug}: draft route published`);
  assert(!existsSync(`dist/firmware/${slug}.bin`), `${slug}: draft firmware published`);
}
console.log(`Verified ${published.length} guide/story pairs, Markdown parity, heading anchors, excerpts, board identities, firmware manifests, and draft exclusion.`);
