# Shared by build.sh and run_tests.sh: firmware sources the runner links.
# They are compiled as-is from Arduino/app/src (read-only for this tool).
APP_SRC="$HERE/../../app/src"
FIRMWARE_SRCS=(
  "$APP_SRC/ui/screen_manager.cpp"
  "$APP_SRC/ui/screens.cpp"
  "$APP_SRC/ui/telescope_ui.cpp"
  "$APP_SRC/motor/ra_motor.cpp"
  "$APP_SRC/estimation/filter.cpp"
  "$APP_SRC/estimation/attitude_estimator.cpp"
  "$APP_SRC/astro/astro.cpp"
  "$APP_SRC/config/settings.cpp"
)
# stubs/ first so <Arduino.h> resolves to the simulator's own stub
INCLUDES=(-I "$HERE/stubs" -I "$APP_SRC" -I "$HERE/runner")
CXXFLAGS=(-std=c++14 -Wall -Wextra -Wno-unused-parameter -Wno-format-nonliteral)
