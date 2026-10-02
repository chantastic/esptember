#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
DAY="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# Contract: three reducer shapes, the shared runner, and a mutation that must fail.
"$ROOT/scripts/prompt-contract/validate_day.sh" "$DAY"
# Domain: every expectation matches SPEC's rules; the frame vectors decode as specified.
python3 "$DAY/tests/reference_model.py" "$DAY/tests/contract.json"
python3 "$DAY/tests/check_frames.py"
# Host bridge, when a host candidate exists.
HOST="$DAY/.build/host-candidate/agent-dial"
if [[ -f "$HOST/hub.mjs" ]]; then node "$DAY/tests/check_host.mjs" "$HOST"; else echo "No host candidate; skipped host bridge checks."; fi
