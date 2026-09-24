#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "Usage: $0 DAY_DIRECTORY" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DAY="$(cd "$1" && pwd)"
CONTRACT="$DAY/tests/contract.json"
OUTPUT="$DAY/.build/contract-candidates"
python3 "$ROOT/scripts/prompt-contract/generate_candidates.py" "$CONTRACT" "$OUTPUT"
for candidate in "$OUTPUT"/a-list.py "$OUTPUT"/b-table.py "$OUTPUT"/c-functions.py; do
  python3 "$ROOT/scripts/prompt-contract/check_contract.py" "$CONTRACT" "$candidate"
done
MUTATED="$DAY/.build/mutated-candidate"
python3 "$ROOT/scripts/prompt-contract/generate_candidates.py" --mutate "$CONTRACT" "$MUTATED"
if python3 "$ROOT/scripts/prompt-contract/check_contract.py" "$CONTRACT" "$MUTATED/a-list.py" >/dev/null 2>&1; then
  echo "Mutation unexpectedly passed: $DAY" >&2
  exit 1
fi
echo "Mutation rejected: $(basename "$DAY")"
