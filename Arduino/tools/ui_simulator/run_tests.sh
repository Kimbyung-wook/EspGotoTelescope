#!/usr/bin/env bash
# Build and run the runner's menu/protocol tests (host, MinGW g++ C++14).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${UI_SIM_OUT:-$HERE/build}"
mkdir -p "$OUT"
CXX="${CXX:-g++}"
# Everything in runner/ except main.cpp
SRCS=()
for f in "$HERE"/runner/*.cpp; do
  [ "$(basename "$f")" = "main.cpp" ] || SRCS+=("$f")
done
"$CXX" -std=c++14 -Wall -Wextra -Wno-unused-parameter -Wno-format-nonliteral \
  -I "$HERE/runner" "$HERE"/tests/test_*.cpp "${SRCS[@]}" -o "$OUT/ui_tests.exe"
"$OUT/ui_tests.exe"
