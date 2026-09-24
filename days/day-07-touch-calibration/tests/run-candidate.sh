#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 CANDIDATE_DIRECTORY" >&2
  exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CANDIDATE="$(cd "$1" && pwd)"
OUT="$CANDIDATE/.contract-test"

for file in touch_calibration_core.h touch_calibration_core.cpp; do
  if [[ ! -f "$CANDIDATE/$file" ]]; then
    echo "Missing $CANDIDATE/$file" >&2
    exit 2
  fi
done

"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$CANDIDATE" \
  "$ROOT/tests/test_contract.cpp" \
  "$CANDIDATE/touch_calibration_core.cpp" \
  -o "$OUT"

"$OUT"
