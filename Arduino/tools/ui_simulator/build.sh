#!/usr/bin/env bash
# Build the UI simulator runner (host, MinGW g++ C++14).
# Output: build/ui_runner.exe (override with UI_SIM_OUT=dir)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${UI_SIM_OUT:-$HERE/build}"
mkdir -p "$OUT"
CXX="${CXX:-g++}"
"$CXX" -std=c++14 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-format-nonliteral \
  -I "$HERE/runner" "$HERE"/runner/*.cpp -o "$OUT/ui_runner.exe"
echo "built $OUT/ui_runner.exe"
