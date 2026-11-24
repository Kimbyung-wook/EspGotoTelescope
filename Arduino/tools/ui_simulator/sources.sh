# Shared by build.sh and run_tests.sh: firmware sources the runner links.
# They are compiled as-is from Arduino/app/src (read-only for this tool).
APP_SRC="$HERE/../../app/src"
FIRMWARE_SRCS=(
  "$APP_SRC/ui/screen_manager.cpp"
  "$APP_SRC/ui/screens/menu.cpp"
  "$APP_SRC/ui/screens/status.cpp"
  "$APP_SRC/ui/screens/ra_speed.cpp"
  "$APP_SRC/ui/screens/calibration.cpp"
  "$APP_SRC/ui/screens/polar_align.cpp"
  "$APP_SRC/ui/screens/target.cpp"
  "$APP_SRC/ui/screens/settings.cpp"
  "$APP_SRC/ui/simplegoto_ui.cpp"
  "$APP_SRC/motor/ra_motor.cpp"
  "$APP_SRC/estimation/filter.cpp"
  "$APP_SRC/estimation/attitude_estimator.cpp"
  "$APP_SRC/estimation/calibration.cpp"
  "$APP_SRC/location/location.cpp"
  "$APP_SRC/astro/astro.cpp"
  "$APP_SRC/config/settings.cpp"
  "$APP_SRC/app/core.cpp"
)
# stubs/ first so <Arduino.h> resolves to the simulator's own stub
INCLUDES=(-I "$HERE/stubs" -I "$APP_SRC" -I "$HERE/runner")
CXXFLAGS=(-std=c++14 -Wall -Wextra -Wno-unused-parameter -Wno-format-nonliteral)
