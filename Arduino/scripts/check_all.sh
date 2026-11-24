#!/usr/bin/env bash
# Run every check in order and stop at the first failure:
#   unit tests, UI simulator (build + tests), protocol check, remote page check,
#   firmware build (default and remote UI). Prints IRAM/IROM sizes and code
#   metrics (scripts/metrics.py) at the end.
# Usage: Arduino/scripts/check_all.sh [--no-firmware]
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
FIRMWARE=1
[ "${1:-}" = "--no-firmware" ] && FIRMWARE=0

step() { echo; echo "=== $* ==="; }

step "unit tests"
bash "$HERE/test/run_tests.sh" | tail -n 3

step "UI simulator build"
bash "$HERE/tools/ui_simulator/build.sh"

step "UI simulator tests"
bash "$HERE/tools/ui_simulator/run_tests.sh" | tail -n 3

step "protocol check"
python "$HERE/tools/ui_simulator/gui/protocol_check.py" | tail -n 3

step "remote page check"
python "$HERE/scripts/gen_remote_page.py" --check

[ "$FIRMWARE" = 1 ] || { echo; echo "ALL OK (firmware skipped)"; exit 0; }

sizes=()
build() {  # build <label> [arduino-cli args]
  local label="$1"; shift
  step "firmware build: $label"
  local log
  log="$(bash "$HERE/scripts/build.sh" "$@" 2>&1)" || { echo "$log"; exit 1; }
  # ESP8266 core prints segment rows like "╚══ IRAM     30343    code in IRAM"
  local iram irom
  iram="$(echo "$log" | grep -oE 'IRAM +[0-9]+' | grep -oE '[0-9]+' | head -n1 || true)"
  irom="$(echo "$log" | grep -oE 'IROM +[0-9]+' | grep -oE '[0-9]+' | head -n1 || true)"
  echo "IRAM ${iram:-?}  IROM ${irom:-?}"
  sizes+=("$(printf '%-10s IRAM %6s / 32768   IROM %7s' "$label" "${iram:-?}" "${irom:-?}")")
}
build default
build remote_ui --build-property "compiler.cpp.extra_flags=-DREMOTE_UI_ENABLE=1"

echo
echo "=== sizes ==="
printf '%s\n' "${sizes[@]}"
echo "ALL OK"
