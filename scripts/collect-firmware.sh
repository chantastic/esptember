#!/usr/bin/env bash
# Copy each day's merged firmware binary into public/firmware/<day>.bin
# so the site serves flashable images alongside the day's page.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p public/firmware

for readme in days/*/README.md; do
  # Only publish firmware for days with a published lesson.
  [ -f "$readme" ] || continue
  # Prompt-only lessons may keep disposable candidates under .build, but those
  # are evidence rather than release artifacts.
  grep -q '^firmware: ' "$readme" || continue
  day_root="${readme%/README.md}"
  day="${day_root##*/}"
  merged=""
  # ESP-IDF and Arduino CLI keep their merged fresh-install images here.
  for candidate in "$day_root/firmware/build/merged-binary.bin" "$day_root"/.build/firmware/*.ino.merged.bin; do
    [ -f "$candidate" ] || continue
    if [ -n "$merged" ]; then
      echo "Multiple merged firmware images found for $day; choose one before publishing." >&2
      exit 1
    fi
    merged="$candidate"
  done
  [ -n "$merged" ] || continue
  cp "$merged" "public/firmware/$day.bin"
  echo "collected: public/firmware/$day.bin"
done
