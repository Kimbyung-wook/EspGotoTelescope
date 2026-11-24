#include "test_main.h"
#include "../app/src/estimation/calibration.h"
#include "../app/src/estimation/attitude_estimator.h"

using namespace cal;

namespace {
ImuData still(float wx = 0.3f, float wy = -0.2f, float wz = 0.1f) {
  ImuData d; d.az = GRAVITY; d.wx = wx; d.wy = wy; d.wz = wz;
  return d;
}
// Pose with axis (0 x, 1 y, 2 z) pointing up (sign +1) or down (-1), reading
// v = s * g * sign + b on that axis
ImuData pose(int axis, float sign, const float b[3], const float s[3]) {
  ImuData d;
  float v[3] = {b[0], b[1], b[2]};
  v[axis] += s[axis] * GRAVITY * sign;
  d.ax = v[0]; d.ay = v[1]; d.az = v[2];
  return d;
}
void feedAccel(AccelCalibration& c, unsigned long& ms, unsigned long dur, const ImuData& d) {
  for (unsigned long end = ms + dur; ms < end; ms += 100) c.update(ms, d);
}
}

TEST(cal_shock_detector_flags_a_jump) {
  ShockDetector s(2.0f, 0.5f);
  CHECK(!s.update(still()));
  CHECK(!s.update(still(0.4f, -0.2f, 0.1f)));        // small noise
  CHECK(s.update(still(5.0f, 0.0f, 0.0f)));          // |w| jumps by more than 2 dps
  ImuData bump = still(); bump.az = GRAVITY + 1.0f;  // |a| by more than 0.5
  CHECK(s.update(bump));
}

TEST(cal_gyro_settle_measure_result) {
  GyroCalibration c;
  CHECK(c.state() == GyroCalibration::IDLE);
  c.start(1000);
  c.update(1500, still(9.0f, 9.0f, 9.0f));           // inside the 1 s settle : ignored
  CHECK(c.state() == GyroCalibration::SETTLE);
  int k = 0;
  for (unsigned long ms = 2000; ms <= 32000; ms += 100, k++) {
    const float n = (k & 1) ? 0.01f : -0.01f;         // noise +-0.01
    c.update(ms, still(0.3f + n, -0.2f, 0.1f));
    if (ms == 14000) CHECK(c.elapsedS() == 12);
  }
  CHECK(c.state() == GyroCalibration::RESULT);
  CHECK(c.samples() == 301);
  CHECK_NEAR(c.bias(0), 0.3, 1e-4);
  CHECK_NEAR(c.bias(1), -0.2, 1e-5);
  CHECK_NEAR(c.stddev(0), 0.01, 1e-3);
  CHECK_NEAR(c.stddev(1), 0.0, 1e-6);
}

TEST(cal_gyro_shock_fails_and_retry_starts_over) {
  GyroCalibration c;
  c.start(0);
  for (unsigned long ms = 1000; ms < 5000; ms += 100) c.update(ms, still());
  CHECK(c.state() == GyroCalibration::MEASURE);
  c.update(5000, still(10.0f, 0.0f, 0.0f));           // someone touched the mount
  CHECK(c.state() == GyroCalibration::FAILED);
  c.update(5100, still());                            // stays failed
  CHECK(c.state() == GyroCalibration::FAILED);
  c.start(6000);
  CHECK(c.state() == GyroCalibration::SETTLE);
  CHECK(c.samples() == 0);
}

TEST(cal_accel_solve_bias_and_scale) {
  const float up[3] = {9.90f, 9.70f, 9.80665f + 0.2f}, down[3] = {-9.70f, -9.90f, -9.80665f + 0.2f};
  float b[3], s[3];
  CHECK(AccelCalibration::solve(up, down, b, s));
  CHECK_NEAR(b[0], 0.1, 1e-5);
  CHECK_NEAR(b[1], -0.1, 1e-5);
  CHECK_NEAR(b[2], 0.2, 1e-5);
  CHECK_NEAR(s[0], 19.6 / (2 * 9.80665), 1e-5);
  CHECK_NEAR(s[2], 1.0, 1e-5);
  const float bad_up[3] = {12.0f, 9.8f, 9.8f};       // scale 1.12
  CHECK(!AccelCalibration::solve(bad_up, down, b, s));
}

TEST(cal_accel_pose_check) {
  const float b[3] = {0, 0, 0}, s[3] = {1, 1, 1};
  CHECK(AccelCalibration::poseOk(0, pose(0, +1, b, s)));
  CHECK(!AccelCalibration::poseOk(1, pose(0, +1, b, s)));   // X- wanted
  CHECK(AccelCalibration::poseOk(5, pose(2, -1, b, s)));
  ImuData tilted = pose(2, +1, b, s); tilted.ax = 5.0f;     // half way
  CHECK(!AccelCalibration::poseOk(4, tilted));
}

TEST(cal_accel_six_positions_and_correction) {
  const float b[3] = {0.12f, -0.08f, 0.25f}, s[3] = {1.01f, 0.99f, 1.02f};
  AccelCalibration c;
  unsigned long ms = 0;
  c.start(ms);
  CHECK(c.state() == AccelCalibration::ORIENT && c.step() == 0);
  feedAccel(c, ms, 2000, pose(2, +1, b, s));          // wrong pose: waits
  CHECK(c.state() == AccelCalibration::ORIENT && c.step() == 0);
  for (int step = 0; step < AccelCalibration::STEPS; step++) {
    const ImuData d = pose(step / 2, step % 2 == 0 ? 1.0f : -1.0f, b, s);
    feedAccel(c, ms, 600, d);
    CHECK(c.state() == AccelCalibration::SETTLE);
    feedAccel(c, ms, 6000, d);
    CHECK(c.step() == step + 1);
  }
  CHECK(c.state() == AccelCalibration::RESULT);
  for (int i = 0; i < 3; i++) {
    CHECK_NEAR(c.bias(i), b[i], 1e-4);
    CHECK_NEAR(c.scale(i), s[i], 1e-5);
  }
  float cb[3], cs[3];
  for (int i = 0; i < 3; i++) { cb[i] = c.bias(i); cs[i] = c.scale(i); }
  const ImuData level = withAccelCalibration(pose(2, +1, b, s), cb, cs);
  CHECK_NEAR(level.ax, 0.0, 1e-4);
  CHECK_NEAR(level.az, GRAVITY, 1e-4);
}

TEST(cal_accel_shock_redoes_only_that_step) {
  const float b[3] = {0, 0, 0}, s[3] = {1, 1, 1};
  AccelCalibration c;
  unsigned long ms = 0;
  c.start(ms);
  feedAccel(c, ms, 7000, pose(0, +1, b, s));          // X+ done
  CHECK(c.step() == 1);
  const ImuData xm = pose(0, -1, b, s);
  feedAccel(c, ms, 3000, xm);                         // X- measuring
  CHECK(c.state() == AccelCalibration::MEASURE);
  ImuData knock = xm; knock.wx = 20.0f;
  c.update(ms, knock); ms += 100;
  CHECK(c.state() == AccelCalibration::ORIENT);
  CHECK(c.step() == 1);                               // same step, not from the start
  CHECK(c.shockInStep());
  feedAccel(c, ms, 7000, xm);
  CHECK(c.step() == 2);
  CHECK(!c.shockInStep());
}

TEST(cal_accel_bad_result_fails) {
  const float b[3] = {1.5f, 0, 0}, s[3] = {1, 1, 1};  // bias 1.5 m/s^2 > 1
  AccelCalibration c;
  unsigned long ms = 0;
  c.start(ms);
  for (int step = 0; step < AccelCalibration::STEPS; step++)
    feedAccel(c, ms, 7000, pose(step / 2, step % 2 == 0 ? 1.0f : -1.0f, b, s));
  CHECK(c.state() == AccelCalibration::FAILED);
}

TEST(bias_removed_from_gyro_only) {
  ImuData d; d.ax = 1; d.wx = 2; d.wy = 3; d.wz = 4;
  const float b[3] = {0.5f, 1.0f, 1.5f};
  ImuData c = withoutGyroBias(d, b);
  CHECK_NEAR(c.ax, 1.0, 1e-6);
  CHECK_NEAR(c.wx, 1.5, 1e-6);
  CHECK_NEAR(c.wy, 2.0, 1e-6);
  CHECK_NEAR(c.wz, 2.5, 1e-6);
}

// Tilted still mount : estimate settles on the accelerometer tilt
TEST(attitude_estimator_settles_on_gravity_tilt) {
  const float g = 9.80665f, r = 10.0f * 3.14159265f / 180.0f;
  ImuData d; d.ay = g * std::sin(r); d.az = g * std::cos(r);
  AttitudeEstimator est; est.begin(1.0f, 0.1f);
  for (int k = 0; k < 200; k++) est.update(d);
  CHECK_NEAR(est.rollDeg(), 10.0, 1e-2);
  CHECK_NEAR(est.pitchDeg(), 0.0, 1e-2);
}

// Fast rotation : the gyro carries the angle before the accelerometer catches up
TEST(attitude_estimator_follows_gyro_short_term) {
  ImuData d; d.az = 9.80665f;          // accelerometer still says level
  AttitudeEstimator est; est.begin(10.0f, 0.1f);
  est.update(d);
  d.wy = 10.0f;                         // 10 dps for 1 s
  for (int k = 0; k < 10; k++) est.update(d);
  CHECK(est.pitchDeg() > 8.0f);
  CHECK(est.pitchDeg() < 10.5f);
}
