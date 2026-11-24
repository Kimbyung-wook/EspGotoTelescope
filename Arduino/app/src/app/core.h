// One frame of the main loop without the hardware, used by app.ino on the board
// and by the UI simulator (tools/ui_simulator/runner/firmware_ui.cpp), so both
// run the same steps in the same order:
//   step()   : sensor values -> correctImu -> attitude estimator -> phone clock
//              -> updateLocationTime -> updateAstro -> SimpleGoToUi::step
//   (caller) : show the frame (OLED, phone mirror)
//   finish() : settings requests (save / reset) -> RA motor command
// Pure C++ (no Arduino calls).
#pragma once
#ifndef _APP_CORE_H_
#define _APP_CORE_H_
#include "app_state.h"
#include "../astro/target_tracker.h"
#include "../config/settings.h"
#include "../estimation/attitude_estimator.h"
#include "../location/location.h"
#include "../ui/frame.h"
#include "../ui/simplegoto_ui.h"

namespace app {

// What the hardware (or the simulator) gives for one frame
struct FrameInput {
  unsigned long now_ms = 0;
  int  delta = 0;                 // rotary steps since the last frame (phone dial added)
  bool click = false;             // button or phone click
  GpsRaw  gps;
  ImuData imu;                    // raw, body frame
  PowerState power;
  int remote = Telemetry::REMOTE_OFF;
  HealthState health;             // est_us / astro_us: see Core(micros)
  // Phone clock from the remote UI page since the last frame (DES-LOC-05)
  bool phone_time = false;
  unsigned long phone_unix_s = 0;
  int  phone_ms = 0;
};

class Core {
 public:
  static constexpr float ATT_EST_TAU_S = 1.0f;   // complementary filter crossover

  // micros: clock used to time the estimator and the 1 s astro update
  // (HealthState est_us / astro_us). nullptr: those come from FrameInput::health.
  using MicrosFn = unsigned long (*)();
  explicit Core(float loop_hz, MicrosFn micros = nullptr);

  // setup(): settings into UiState::cfg (defaults on any problem), estimator start
  config::LoadResult begin(config::SettingsStore& store);
  // loop(): everything up to the rendered frame
  void step(const FrameInput& in, Frame& frame);
  // loop(), after the frame is shown: Save / Reset requests, then the RA motor command
  float finish(config::SettingsStore& store);

  bool displayOn() const { return !ctx_.state.display_off; }   // Sleep turns the OLED off
  UiContext& context() { return ctx_; }
  SimpleGoToUi& ui() { return ui_; }

 private:
  float loop_hz_;
  MicrosFn micros_;
  unsigned long est_us_ = 0, astro_us_ = 0;   // last measured (shown one frame later)
  SimpleGoToUi ui_;
  UiContext ctx_;
  AttitudeEstimator att_est_;
  astro::TargetTracker moon_tracker_;     // Moon (lunar rate), once per second
  astro::TargetTracker target_tracker_;   // 1Target, once per second
  loc::LastGpsRecorder last_gps_;         // DES-LOC-02
};

}  // namespace app

#endif // _APP_CORE_H_
