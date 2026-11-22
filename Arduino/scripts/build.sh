#!/usr/bin/env bash
# Build the firmware with the arduino-cli bundled in Arduino IDE 2.
# Usage: Arduino/scripts/build.sh [extra arduino-cli compile args, e.g. --upload -p COM3]
#   SKETCH=tools/astro_bench Arduino/scripts/build.sh --library app/src/astro   (benchmark sketch)
#   FQBN=esp8266:esp8266:nodemcuv2 Arduino/scripts/build.sh                     (80 MHz CPU)
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
CLI="${ARDUINO_CLI:-$LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe}"
CFG="${ARDUINO_CLI_CONFIG:-$USERPROFILE/.arduinoIDE/arduino-cli.yaml}"
# xtal=160 : 160 MHz CPU (Arduino IDE: Tools > CPU Frequency > 160 MHz)
FQBN="${FQBN:-esp8266:esp8266:nodemcuv2:xtal=160}"
SKETCH="${SKETCH:-$HERE/app}"
case "$SKETCH" in /*|?:*) ;; *) SKETCH="$HERE/$SKETCH" ;; esac
args=()
for a in "$@"; do  # resolve --library paths relative to Arduino/
  if [ "${prev:-}" = "--library" ]; then case "$a" in /*|?:*) ;; *) a="$HERE/$a" ;; esac; fi
  args+=("$a"); prev="$a"
done
"$CLI" --config-file "$CFG" compile -b "$FQBN" --warnings default "$SKETCH" "${args[@]}"
