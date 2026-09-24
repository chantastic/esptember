#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "Usage: $0 CANDIDATE_DIRECTORY" >&2
  exit 2
fi
DAY_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CANDIDATE="$(cd "$1" && pwd)"
mkdir -p "$DAY_ROOT/.build/tests"
for source in "$DAY_ROOT"/tests/check_*.cpp; do
  binary="$DAY_ROOT/.build/tests/$(basename "${source%.cpp}")"
  clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -g -I "$CANDIDATE" "$source" -o "$binary"
  "$binary"
done
