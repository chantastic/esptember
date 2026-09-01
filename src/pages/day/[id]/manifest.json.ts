import type { APIRoute } from "astro";
import { getCollection, type CollectionEntry } from "astro:content";

// ESP Web Tools manifest, one per day that ships firmware.
// https://esphome.github.io/esp-web-tools/

export async function getStaticPaths() {
  const days = await getCollection("days");
  return days
    .filter((entry) => entry.data.firmware)
    .map((entry) => ({ params: { id: entry.id }, props: { entry } }));
}

export const GET: APIRoute = ({ props }) => {
  const entry = props.entry as CollectionEntry<"days">;
  const manifest = {
    name: `ESPtember day ${String(entry.data.day).padStart(2, "0")} — ${entry.data.title}`,
    version: "1",
    builds: [
      {
        chipFamily: "ESP32-S3",
        parts: [{ path: entry.data.firmware, offset: 0 }],
      },
    ],
  };
  return new Response(JSON.stringify(manifest, null, 2), {
    headers: { "Content-Type": "application/json" },
  });
};
