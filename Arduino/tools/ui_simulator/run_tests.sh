#!/usr/bin/env bash
# Build and run the runner's menu/protocol tests against the real firmware UI.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
source "$HERE/sources.sh"
OUT="${UI_SIM_OUT:-$HERE/build}"
mkdir -p "$OUT"
CXX="${CXX:-g++}"
# Everything in runner/ except main.cpp
SRCS=()
for f in "$HERE"/runner/*.cpp; do
  [ "$(basename "$f")" = "main.cpp" ] || SRCS+=("$f")
done
"$CXX" "${CXXFLAGS[@]}" "${INCLUDES[@]}" \
  "$HERE"/tests/test_*.cpp "${SRCS[@]}" "${FIRMWARE_SRCS[@]}" -o "$OUT/ui_tests.exe"
"$OUT/ui_tests.exe"
# The phone page embedded in the firmware must match web/index.html
python "$HERE/../../scripts/gen_remote_page.py" --check
