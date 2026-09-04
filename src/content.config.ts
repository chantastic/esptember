import { defineCollection, z } from "astro:content";
import { glob } from "astro/loaders";
import boards from "./lib/boards.json";
const boardIds = Object.keys(boards) as [keyof typeof boards, ...(keyof typeof boards)[]];

const days = defineCollection({
  loader: glob({ base: "./days", pattern: "*/README.md", generateId: ({ entry }) => entry.split("/")[0]! }),
  schema: z.object({
    board: z.enum(boardIds),
    day: z.number(), title: z.string(), summary: z.string(),
    verification: z.string(), toolchain: z.string(),
    firmware: z.string().optional(), video: z.string().optional(),
  }),
});
const stories = defineCollection({
  loader: glob({ base: "./days", pattern: "*/STORY.md", generateId: ({ entry }) => entry.split("/")[0]! }),
});
export const collections = { days, stories };
