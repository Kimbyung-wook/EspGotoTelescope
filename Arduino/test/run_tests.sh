#!/usr/bin/env bash
# Build and run host (PC) unit tests. Needs g++ (C++14).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../app/src"
OUT="${TEST_OUT:-$HERE/build}"
mkdir -p "$OUT"
g++ -std=c++14 -Wall -Wextra -Wno-unused-parameter -I "$HERE/stubs" -I "$HERE" \
  "$HERE"/test_*.cpp \
  "$SRC/estimation/filter.cpp" \
  ${EXTRA_SRCS:-} \
  -o "$OUT/unit_tests.exe"
"$OUT/unit_tests.exe"
