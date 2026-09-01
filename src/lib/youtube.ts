/** Accepts a YouTube URL (watch, youtu.be, shorts) or a bare video ID. */
export function youtubeId(input: string): string {
  const m = input.match(
    /(?:youtube\.com\/(?:watch\?.*v=|shorts\/|embed\/)|youtu\.be\/)([\w-]{11})/,
  );
  return m?.[1] ?? input;
}
