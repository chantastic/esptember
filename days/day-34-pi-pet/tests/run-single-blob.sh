#!/usr/bin/env bash
set -euo pipefail
candidate="${1:?Pass the disposable single_blob candidate directory}"
test -f "$candidate/single_blob_motion.h"
output="$(mktemp -t single-blob-contract)"
trap 'unlink "$output"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror \
  -I "$candidate" \
  "$(dirname "$0")/single_blob_motion.cpp" -o "$output"
"$output"
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror \
  -I "$candidate" \
  "$(dirname "$0")/single_blob_touch_map.cpp" -o "$output"
"$output"
echo 'Single-blob motion contract passed.'
