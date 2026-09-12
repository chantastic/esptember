#!/usr/bin/env bash
set -euo pipefail
DAY_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$DAY_ROOT/.build/tests"
for source in "$DAY_ROOT"/tests/check_*.cpp; do
  binary="$DAY_ROOT/.build/tests/$(basename "${source%.cpp}")"
  clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -g "$source" -o "$binary"
  "$binary"
done
