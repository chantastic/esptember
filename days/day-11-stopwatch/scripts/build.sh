#!/usr/bin/env bash
set -euo pipefail
DAY_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARDUINO_CLI="${ARDUINO_CLI:-arduino-cli}"
FQBN='esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi'
mkdir -p "$DAY_ROOT/.build/firmware"
"$ARDUINO_CLI" compile --fqbn "$FQBN" --warnings default --export-binaries=false \
  --output-dir "$DAY_ROOT/.build/firmware" "$DAY_ROOT/firmware/stopwatch"
