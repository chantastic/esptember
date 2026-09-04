#!/usr/bin/env bash
# Copy each day's merged firmware binary into public/firmware/<day>.bin
# so the site serves flashable images alongside the day's page.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p public/firmware

for merged in days/*/firmware/build/merged-binary.bin; do
  [ -f "$merged" ] || continue
  day="$(basename "$(dirname "$(dirname "$(dirname "$merged")")")")"
  # Only publish firmware for days with a published lesson.
  [ -f "days/$day/README.md" ] || continue
  cp "$merged" "public/firmware/$day.bin"
  echo "collected: public/firmware/$day.bin"
done
