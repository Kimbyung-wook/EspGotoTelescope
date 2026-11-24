#include "calibration.h"
#include <math.h>
#include "../board_config.h"

namespace cal {

/************************************/
// Shock and statistics
/************************************/
bool ShockDetector::update(const ImuData& d) {
  const float w = sqrtf(d.wx * d.wx + d.wy * d.wy + d.wz * d.wz);
  const float a = accelNorm(d);
  if (n_ > 0 && (fabsf(w - w_mean_) > gyro_lim_ || fabsf(a - a_mean_) > accel_lim_)) return true;
  n_++;
  w_mean_ += (w - w_mean_) / (float)n_;
  a_mean_ += (a - a_mean_) / (float)n_;
  return false;
}

void Stats3::add(const float v[3]) {
  n_++;
  for (int i = 0; i < 3; i++) {
    const double d = v[i] - mean_[i];
    mean_[i] += d / (double)n_;
    m2_[i] += d * (v[i] - mean_[i]);
  }
}

float Stats3::stddev(int i) const {
  return n_ > 1 ? (float)sqrt(m2_[i] / (double)(n_ - 1)) : 0.0f;
}

/************************************/
// Gyro
/************************************/
GyroCalibration::GyroCalibration() : shock_(CAL_SHOCK_GYRO_DPS, CAL_SHOCK_ACCEL_MPS2) {}

void GyroCalibration::start(unsigned long now_ms) {
  state_ = SETTLE;
  start_ms_ = now_ms;
  measured_ms_ = 0;
  shock_.reset();
  stats_.reset();
}

void GyroCalibration::update(unsigned long now_ms, const ImuData& raw) {
  if (state_ != SETTLE && state_ != MEASURE) return;
  const unsigned long elapsed = now_ms - start_ms_;
  if (elapsed < SETTLE_MS) return;                 // not judged, not averaged
  state_ = MEASURE;
  if (shock_.update(raw)) { state_ = FAILED; return; }
  const float w[3] = {raw.wx, raw.wy, raw.wz};
  stats_.add(w);
  measured_ms_ = elapsed - SETTLE_MS;
  if (measured_ms_ >= MEASURE_MS) state_ = RESULT;
}

/************************************/
// Accelerometer
/************************************/
AccelCalibration::AccelCalibration() : shock_(CAL_SHOCK_GYRO_DPS, CAL_SHOCK_ACCEL_MPS2) {}

const char* AccelCalibration::stepName(int step) {
  static const char* const names[STEPS] = {"X+", "X-", "Y+", "Y-", "Z+", "Z-"};
  return (step >= 0 && step < STEPS) ? names[step] : "--";
}

bool AccelCalibration::poseOk(int step, const ImuData& d) {
  if (step < 0 || step >= STEPS) return false;
  const float v[3] = {d.ax, d.ay, d.az};
  const int axis = step / 2;
  const float sign = (step % 2 == 0) ? 1.0f : -1.0f;
  if (sign * v[axis] < POSE_UP_G * GRAVITY) return false;
  for (int i = 0; i < 3; i++)
    if (i != axis && fabsf(v[i]) > POSE_OTHER_G * GRAVITY) return false;
  return true;
}

bool AccelCalibration::solve(const float up[3], const float down[3], float bias[3], float scale[3]) {
  bool ok = true;
  for (int i = 0; i < 3; i++) {
    bias[i] = (up[i] + down[i]) * 0.5f;
    scale[i] = (up[i] - down[i]) / (2.0f * GRAVITY);
    if (!(scale[i] >= SCALE_MIN && scale[i] <= SCALE_MAX)) ok = false;
    if (!(fabsf(bias[i]) <= BIAS_MAX)) ok = false;
  }
  return ok;
}

void AccelCalibration::start(unsigned long now_ms) {
  state_ = ORIENT;
  step_ = 0;
  phase_ms_ = now_ms;
  shock_seen_ = false;
  measured_ms_ = 0;
  for (int i = 0; i < 3; i++) { up_[i] = down_[i] = 0.0f; bias_[i] = 0.0f; scale_[i] = 1.0f; }
}

void AccelCalibration::restartStep(bool shock) {
  state_ = ORIENT;
  measured_ms_ = 0;
  if (shock) shock_seen_ = true;
}

void AccelCalibration::update(unsigned long now_ms, const ImuData& raw) {
  switch (state_) {
    case ORIENT:
      if (poseOk(step_, raw)) {
        state_ = SETTLE;
        phase_ms_ = now_ms;
        shock_.reset();
        shock_.update(raw);
      }
      return;
    case SETTLE:
    case MEASURE: {
      if (!poseOk(step_, raw) || shock_.update(raw)) { restartStep(true); return; }
      const unsigned long elapsed = now_ms - phase_ms_;
      if (state_ == SETTLE) {
        if (elapsed < SETTLE_MS) return;
        state_ = MEASURE;
        phase_ms_ = now_ms;
        stats_.reset();
      }
      const float v[3] = {raw.ax, raw.ay, raw.az};
      stats_.add(v);
      measured_ms_ = now_ms - phase_ms_;
      if (measured_ms_ < MEASURE_MS) return;
      const int axis = step_ / 2;
      if (step_ % 2 == 0) up_[axis] = stats_.mean(axis);
      else                down_[axis] = stats_.mean(axis);
      step_++;
      shock_seen_ = false;
      measured_ms_ = 0;
      if (step_ < STEPS) { state_ = ORIENT; return; }
      state_ = solve(up_, down_, bias_, scale_) ? RESULT : FAILED;
      return;
    }
    default:
      return;
  }
}

} // namespace cal
