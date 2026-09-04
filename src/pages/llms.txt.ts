import type { APIRoute } from "astro";
import boards from "../lib/boards.json";
import { getCollection } from "astro:content";
export const GET: APIRoute = async () => {
  const days = (await getCollection("days")).sort((a,b) => a.data.day-b.data.day);
  return new Response(`# ESPtember\n\n> Standalone ESP32-S3 projects. Each guide gives prerequisites, commands, expected behavior, known failures, and the scope of recorded verification.\n\nEach guide names its target board and hardware revision. Firmware is board-specific; the series uses multiple dev kits.\nRead the selected guide before changing or flashing firmware. A build or serial heartbeat does not prove visual rendering or touch behavior.\n\n## Published guides\n\n${days.map(entry => `- [Day ${entry.data.day}: ${entry.data.title}](https://esptember.com/day/${entry.id}/guide.md): ${entry.data.summary} Target: ${boards[entry.data.board].model}.`).join('\n')}\n\n## Source\n\n- [Repository](https://github.com/chantastic/esptember): firmware projects, saved media, conversion scripts, and build stories.\n`, { headers: { 'Content-Type': 'text/plain; charset=utf-8' } });
};
