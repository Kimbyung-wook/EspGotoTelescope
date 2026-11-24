#pragma once
#ifndef _CALIBRATION_H_
#define _CALIBRATION_H_
#include "../sensors/imu_data.h"

/************************************/
// IMU calibration (docs/REQUIREMENTS.md DES-CAL-01..05)
//   Gyro  : 1 s settle, then 30 s of samples -> mean (bias) and standard deviation
//   Accel : six poses X+ X- Y+ Y- Z+ Z- up, 5 s mean each -> bias and scale per axis
//   Shock : a sample far from the running mean of |w| or |a| spoils the measurement
// Fed with the raw IMU sample (bias not removed) once per frame; time from now_ms.
// Pure C++ (no Arduino calls): host tests and the UI simulator use it as-is.
/************************************/
namespace cal {

static const float GRAVITY = 9.80665f;           // [m/s^2]

// Running mean of |w| and |a|; a sample further than the limits from it is a shock
class ShockDetector {
public:
  ShockDetector(float gyro_dps, float accel_mps2) : gyro_lim_(gyro_dps), accel_lim_(accel_mps2) {}
  void reset() { n_ = 0; w_mean_ = a_mean_ = 0.0f; }
  // Returns true when this sample is a shock (it is then not added to the mean)
  bool update(const ImuData& d);
private:
  float gyro_lim_, accel_lim_;
  unsigned long n_ = 0;
  float w_mean_ = 0.0f, a_mean_ = 0.0f;
};

// Mean and standard deviation of 3 axes (Welford)
class Stats3 {
public:
  void reset() { n_ = 0; for (int i = 0; i < 3; i++) mean_[i] = m2_[i] = 0.0; }
  void add(const float v[3]);
  unsigned long count() const { return n_; }
  float mean(int i) const { return (float)mean_[i]; }
  float stddev(int i) const;
private:
  unsigned long n_ = 0;
  double mean_[3] = {0, 0, 0};
  double m2_[3] = {0, 0, 0};
};

/************************************/
// Gyro bias (DES-CAL-03)
/************************************/
class GyroCalibration {
public:
  enum State { IDLE, SETTLE, MEASURE, RESULT, FAILED };
  static const unsigned long SETTLE_MS  = 1000;    // hand on the button
  static const unsigned long MEASURE_MS = 30000;

  GyroCalibration();
  void start(unsigned long now_ms);
  void update(unsigned long now_ms, const ImuData& raw);
  State state() const { return state_; }
  int   elapsedS() const { return (int)(measured_ms_ / 1000); }   // seconds measured so far
  int   totalS() const { return (int)(MEASURE_MS / 1000); }
  // Running values while measuring, final values in RESULT
  float bias(int i) const { return stats_.mean(i); }
  float stddev(int i) const { return stats_.stddev(i); }
  unsigned long samples() const { return stats_.count(); }

private:
  State state_ = IDLE;
  unsigned long start_ms_ = 0, measured_ms_ = 0;
  ShockDetector shock_;
  Stats3 stats_;
};

/************************************/
// Accelerometer six-position (DES-CAL-04, DES-CAL-05)
/************************************/
class AccelCalibration {
public:
  enum State {
    IDLE,
    ORIENT,      // waiting for the pose of this step
    SETTLE,      // pose found, 1 s still
    MEASURE,     // 5 s mean
    RESULT,      // bias/scale ready (Apply / Discard)
    FAILED       // values out of range (Retry / Quit)
  };
  static const int STEPS = 6;
  static const unsigned long SETTLE_MS  = 1000;
  static const unsigned long MEASURE_MS = 5000;
  static constexpr float POSE_UP_G    = 0.8f;   // that axis above 0.8 g
  static constexpr float POSE_OTHER_G = 0.35f;  // the other two below this
  static constexpr float SCALE_MIN = 0.9f, SCALE_MAX = 1.1f;
  static constexpr float BIAS_MAX = 1.0f;        // [m/s^2]

  AccelCalibration();
  void start(unsigned long now_ms);
  void update(unsigned long now_ms, const ImuData& raw);
  State state() const { return state_; }
  int   step() const { return step_; }                 // 0..5
  static const char* stepName(int step);               // "X+", "X-", ...
  // The pose of 'step' is held (axis step/2 points up with sign +/-)
  static bool poseOk(int step, const ImuData& d);
  bool  shockInStep() const { return shock_seen_; }    // this step was restarted after a shock
  int   measuredS() const { return (int)(measured_ms_ / 1000); }
  float bias(int i) const { return bias_[i]; }
  float scale(int i) const { return scale_[i]; }
  // b = (p + n) / 2, s = (p - n) / (2 g); false when out of range
  static bool solve(const float up[3], const float down[3], float bias[3], float scale[3]);

private:
  void restartStep(bool shock);
  State state_ = IDLE;
  int step_ = 0;
  unsigned long phase_ms_ = 0, measured_ms_ = 0;
  bool shock_seen_ = false;
  ShockDetector shock_;
  Stats3 stats_;
  float up_[3] = {0, 0, 0}, down_[3] = {0, 0, 0};   // axis i reading with axis i up / down
  float bias_[3] = {0, 0, 0}, scale_[3] = {1, 1, 1};
};

// Accelerometer with the six-position result applied: (a - b) / s
inline ImuData withAccelCalibration(const ImuData& d, const float bias[3], const float scale[3]) {
  ImuData c = d;
  c.ax = (d.ax - bias[0]) / (scale[0] != 0.0f ? scale[0] : 1.0f);
  c.ay = (d.ay - bias[1]) / (scale[1] != 0.0f ? scale[1] : 1.0f);
  c.az = (d.az - bias[2]) / (scale[2] != 0.0f ? scale[2] : 1.0f);
  return c;
}

} // namespace cal

#endif // _CALIBRATION_H_
