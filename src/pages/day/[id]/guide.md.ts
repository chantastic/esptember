import type { APIRoute } from "astro";
import boards from "../../../lib/boards.json";
import { getCollection, type CollectionEntry } from "astro:content";
export async function getStaticPaths() {
  return (await getCollection("days")).map(entry => ({ params: { id: entry.id }, props: { entry } }));
}
export const GET: APIRoute = ({ props }) => {
  const entry = props.entry as CollectionEntry<"days">;
  return new Response(`# Day ${String(entry.data.day).padStart(2, '0')}: ${entry.data.title}\n\nGuide: https://esptember.com/day/${entry.id}/\nBoard: ${boards[entry.data.board].model}\nBoard reference: ${boards[entry.data.board].url}\nToolchain: ${entry.data.toolchain}\nVerification: ${entry.data.verification}\n\n${entry.body}`, {
    headers: { "Content-Type": "text/markdown; charset=utf-8" },
  });
};
