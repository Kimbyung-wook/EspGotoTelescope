#!/usr/bin/env bash
# Build and run host (PC) unit tests. Needs g++ (C++14).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../app/src"
OUT="${TEST_OUT:-$HERE/build}"
mkdir -p "$OUT"

# Firmware sources that build on the host (no hardware libraries)
SRCS=(
  "$SRC/estimation/filter.cpp"
  "$SRC/estimation/attitude_estimator.cpp"
  "$SRC/input/rotary.cpp"
  "$SRC/motor/ra_motor.cpp"
  "$SRC/ui/screen_manager.cpp"
  "$SRC/ui/screens.cpp"
  "$SRC/ui/telescope_ui.cpp"
)

g++ -std=c++14 -Wall -Wextra -I "$HERE/stubs" -I "$HERE" \
  "$HERE"/test_*.cpp "${SRCS[@]}" -o "$OUT/unit_tests.exe"
"$OUT/unit_tests.exe"
