#include "test_main.h"
#include "../app/src/estimation/bias_calibrator.h"
#include "../app/src/estimation/attitude_estimator.h"

static void feed(GyroBiasCalibrator& c, unsigned long from_ms, unsigned long to_ms,
                 float wx, float wy, float wz) {
  const float w[3] = {wx, wy, wz};
  for (unsigned long t = from_ms; t < to_ms; t += 100) c.update(t, w);
}

TEST(bias_calibrator_phases_follow_time) {
  GyroBiasCalibrator c;
  CHECK(c.phase() == GyroBiasCalibrator::IDLE);
  c.start(5000);
  const float w[3] = {0, 0, 0};
  CHECK(c.update(5000,  w) == GyroBiasCalibrator::READY);
  CHECK(c.update(6000,  w) == GyroBiasCalibrator::COARSE);
  CHECK(c.update(11000, w) == GyroBiasCalibrator::FINE);
  CHECK(c.update(21000, w) == GyroBiasCalibrator::DONE);
  CHECK(c.done());
}

TEST(bias_calibrator_ready_phase_is_mean) {
  GyroBiasCalibrator c; c.start(0);
  const float a[3] = {1, 2, 3}, b[3] = {3, 4, 5};
  c.update(0, a); c.update(100, b);
  CHECK_NEAR(c.bias()[0], 2.0, 1e-6);
  CHECK_NEAR(c.bias()[1], 3.0, 1e-6);
  CHECK_NEAR(c.bias()[2], 4.0, 1e-6);
}

TEST(bias_calibrator_converges_to_constant_rate) {
  GyroBiasCalibrator c; c.start(0);
  feed(c, 0, 17000, 0.5f, -0.25f, 0.125f);
  CHECK(c.done());
  CHECK_NEAR(c.bias()[0],  0.5,   1e-5);
  CHECK_NEAR(c.bias()[1], -0.25,  1e-5);
  CHECK_NEAR(c.bias()[2],  0.125, 1e-5);
}

TEST(bias_calibrator_averages_out_noise) {
  GyroBiasCalibrator c; c.start(0);
  const float w0[3] = {1.2f, 0, 0}, w1[3] = {0.8f, 0, 0};
  int k = 0;
  for (unsigned long t = 0; t < 16000; t += 100, k++) c.update(t, (k & 1) ? w1 : w0);
  CHECK_NEAR(c.bias()[0], 1.0, 0.01);
}

TEST(bias_calibrator_freezes_after_done) {
  GyroBiasCalibrator c; c.start(0);
  feed(c, 0, 16100, 1.0f, 1.0f, 1.0f);
  CHECK(c.done());
  const float big[3] = {100, 100, 100};
  c.update(20000, big);
  CHECK_NEAR(c.bias()[0], 1.0, 1e-5);
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
  for (int k = 0; k < 200; k++) est.update(d, 0.1f);
  CHECK_NEAR(est.rollDeg(), 10.0, 1e-2);
  CHECK_NEAR(est.pitchDeg(), 0.0, 1e-2);
}

// Fast rotation : the gyro carries the angle before the accelerometer catches up
TEST(attitude_estimator_follows_gyro_short_term) {
  ImuData d; d.az = 9.80665f;          // accelerometer still says level
  AttitudeEstimator est; est.begin(10.0f, 0.1f);
  est.update(d, 0.1f);
  d.wy = 10.0f;                         // 10 dps for 1 s
  for (int k = 0; k < 10; k++) est.update(d, 0.1f);
  CHECK(est.pitchDeg() > 8.0f);
  CHECK(est.pitchDeg() < 10.5f);
}
