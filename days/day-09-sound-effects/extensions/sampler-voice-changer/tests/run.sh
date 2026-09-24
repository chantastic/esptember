#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
EXTENSION="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONTRACT="$EXTENSION/tests/contract.json"
OUTPUT="$EXTENSION/.build/contract-candidates"
python3 "$ROOT/scripts/prompt-contract/generate_candidates.py" "$CONTRACT" "$OUTPUT"
for candidate in "$OUTPUT"/a-list.py "$OUTPUT"/b-table.py "$OUTPUT"/c-functions.py; do
  python3 "$ROOT/scripts/prompt-contract/check_contract.py" "$CONTRACT" "$candidate"
done
MUTATED="$EXTENSION/.build/mutated-candidate"
python3 "$ROOT/scripts/prompt-contract/generate_candidates.py" --mutate "$CONTRACT" "$MUTATED"
if python3 "$ROOT/scripts/prompt-contract/check_contract.py" "$CONTRACT" "$MUTATED/a-list.py" >/dev/null 2>&1; then
  echo "Mutation unexpectedly passed: $EXTENSION" >&2
  exit 1
fi
echo "Mutation rejected: sampler-voice-changer"
