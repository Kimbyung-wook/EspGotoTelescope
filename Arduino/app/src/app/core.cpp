#include "core.h"
#include "loop_logic.h"
#include "settings_binding.h"
#include "../board_config.h"

namespace app {

Core::Core(float loop_hz, MicrosFn micros)
    : loop_hz_(loop_hz), micros_(micros),
      moon_tracker_(astro::Target::Moon, 1000),
      target_tracker_(astro::Target::Mercury, 1000) {}

config::LoadResult Core::begin(config::SettingsStore& store) {
  const config::LoadResult r = loadSettings(store, ctx_);
  att_est_.begin(ATT_EST_TAU_S, 1.0f / loop_hz_);
  return r;
}

void Core::step(const FrameInput& in, Frame& frame) {
  Telemetry& t = ctx_.tel;
  UiState& st = ctx_.state;

  // Sensor values
  t.now_ms = in.now_ms;
  t.gps = in.gps;
  t.imu.raw = in.imu;
  t.power = in.power;
  t.remote = in.remote;
  t.health = in.health;
  if (micros_) {
    t.health.est_us = est_us_;
    t.health.astro_us = astro_us_;
  }
  correctImu(t, st);

  // Estimation
  const unsigned long est_start = micros_ ? micros_() : 0;
  att_est_.update(t.imu.corr);
  if (micros_) est_us_ = micros_() - est_start;
  t.imu.roll_est_deg = att_est_.rollDeg();
  t.imu.pitch_est_deg = att_est_.pitchDeg();

  // Phone time from the remote UI page (DES-LOC-05), then position and time in use
  if (in.phone_time) setPhoneClock(st, t.now_ms, in.phone_unix_s, in.phone_ms);
  updateLocationTime(t, st, last_gps_);

  // Moon and target (each only on the frames where its 1 s period has passed),
  // then where the tube points (IMU on the tube, polar-aligned mount)
  const unsigned long astro_start = micros_ ? micros_() : 0;
  const bool astro_ran = updateAstro(moon_tracker_, target_tracker_, t, st,
                                     IMU_TUBE_OPTICAL_AXIS, IMU_TUBE_DEC_AXIS);
  if (micros_ && astro_ran) astro_us_ = micros_() - astro_start;

  frame.clear();
  ui_.step(in.delta, in.click, ctx_, frame);
}

float Core::finish(config::SettingsStore& store) {
  serviceRequests(store, ctx_);                  // Save / Reset from the UI
  // Tracking overrides the manual RRcmd; calibration / Polar Align stop the motor
  return motorCommand(ctx_.tel, ctx_.state);
}

}  // namespace app
