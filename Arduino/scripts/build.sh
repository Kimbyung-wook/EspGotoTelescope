#!/usr/bin/env bash
# Build the firmware with the arduino-cli bundled in Arduino IDE 2.
# Usage: Arduino/scripts/build.sh [extra arduino-cli compile args, e.g. --upload -p COM3]
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
CLI="${ARDUINO_CLI:-$LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe}"
CFG="${ARDUINO_CLI_CONFIG:-$USERPROFILE/.arduinoIDE/arduino-cli.yaml}"
FQBN="${FQBN:-esp8266:esp8266:nodemcuv2}"
"$CLI" --config-file "$CFG" compile -b "$FQBN" --warnings default "$HERE/app" "$@"
