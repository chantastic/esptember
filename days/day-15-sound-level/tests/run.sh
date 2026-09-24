#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
"$ROOT/scripts/prompt-contract/validate_day.sh" "$ROOT/days/day-15-sound-level"
