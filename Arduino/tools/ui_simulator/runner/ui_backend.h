// The one boundary between the simulator runner and "the UI code".
//
// The runner (protocol.cpp) owns time, input accumulation and sensor values.
// A backend only turns them into a frame of text and a motor output.
// Today the backend is MockUi (mock_ui.cpp, a stand-in following docs/UI.md).
// Later a FirmwareUi backend can wrap the real Screen/ScreenManager code from
// Arduino/app/src with host stubs, and be selected with `--backend firmware`.
#pragma once
#include "sim_types.h"

namespace sim {

class UiBackend {
 public:
  virtual ~UiBackend() {}

  // Short name reported in the "hello" message, e.g. "mock".
  virtual const char* name() const = 0;

  // Power-on: start the boot sequence (like setup()). Called once at start
  // and again on every "reset" message.
  virtual void begin(const BootConfig& boot, const SensorState& sensors) = 0;

  // One 10 Hz frame (like one pass of loop()).
  //   now_ms  - simulated milliseconds since begin(), advances kFrameMs per frame
  //   input   - rotary/button state for this frame
  //   sensors - latest fake sensor values
  // The backend must fill all kLines lines of `out` (empty string = blank line)
  // and set `motor` to the current H-bridge output.
  virtual void frame(unsigned long now_ms, const InputSnapshot& input,
                     const SensorState& sensors, Frame& out,
                     MotorOutput& motor) = 0;
};

// Factory for backends compiled into this runner. Returns nullptr if unknown.
UiBackend* createBackend(const char* name);

}  // namespace sim
