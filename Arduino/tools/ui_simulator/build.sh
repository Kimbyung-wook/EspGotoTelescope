#!/usr/bin/env bash
# Build the UI simulator runner (host, MinGW g++ C++14) with the real firmware UI.
# Output: build/ui_runner.exe (override with UI_SIM_OUT=dir)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
source "$HERE/sources.sh"
OUT="${UI_SIM_OUT:-$HERE/build}"
mkdir -p "$OUT"
CXX="${CXX:-g++}"
"$CXX" "${CXXFLAGS[@]}" -O2 "${INCLUDES[@]}" \
  "$HERE"/runner/*.cpp "${FIRMWARE_SRCS[@]}" -o "$OUT/ui_runner.exe"
echo "built $OUT/ui_runner.exe"
