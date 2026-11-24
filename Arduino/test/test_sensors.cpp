#include "test_main.h"
#include "../app/src/sensors/imu_data.h"
#include "../app/src/gps/gps_link.h"

TEST(imu_sensor_to_body_axes_and_dps) {
  const float a[3] = {1.0f, 2.0f, 3.0f};
  const float w[3] = {0.1f, 0.2f, 0.3f};      // rad/s
  const float m[3] = {10.0f, 20.0f, 30.0f};
  ImuData d = imuSensorToBody(a, w, m, 25.0f);
  CHECK_NEAR(d.ax, -1.0, 1e-6);
  CHECK_NEAR(d.ay,  2.0, 1e-6);
  CHECK_NEAR(d.az,  3.0, 1e-6);
  CHECK_NEAR(d.wx,  0.1 * 57.29578, 1e-3);
  CHECK_NEAR(d.wy, -0.2 * 57.29578, 1e-3);
  CHECK_NEAR(d.wz, -0.3 * 57.29578, 1e-3);
  CHECK_NEAR(d.mx, 10.0, 1e-6);
  CHECK_NEAR(d.temperature, 25.0, 1e-6);
}

TEST(tilt_level_is_zero) {
  ImuData d; d.ax = 0; d.ay = 0; d.az = 9.80665f;
  CHECK_NEAR(rollFromAccelDeg(d), 0.0, 1e-4);
  CHECK_NEAR(pitchFromAccelDeg(d), 0.0, 1e-4);
  CHECK_NEAR(accelNorm(d), 9.80665, 1e-4);
}

TEST(tilt_roll_30deg) {
  ImuData d; d.ax = 0;
  d.ay = 9.80665f * 0.5f; d.az = 9.80665f * 0.8660254f;
  CHECK_NEAR(rollFromAccelDeg(d), 30.0, 1e-3);
  CHECK_NEAR(pitchFromAccelDeg(d), 0.0, 1e-3);
}

TEST(tilt_pitch_37_5deg_like_latitude) {
  const double p = 37.5 * 3.14159265358979 / 180.0;
  ImuData d; d.ax = (float)(-9.80665 * std::sin(p)); d.ay = 0; d.az = (float)(9.80665 * std::cos(p));
  CHECK_NEAR(pitchFromAccelDeg(d), 37.5, 1e-3);
}

TEST(tilt_zero_accel_does_not_nan) {
  ImuData d;
  CHECK(!std::isnan(pitchFromAccelDeg(d)));
}

TEST(gps_link_alive_rules) {
  CHECK(gpsLinkAlive(0, 1000, 3000) == false);      // nothing received yet
  CHECK(gpsLinkAlive(1000, 2000, 3000) == true);
  CHECK(gpsLinkAlive(1000, 4000, 3000) == true);     // exactly at timeout
  CHECK(gpsLinkAlive(1000, 4001, 3000) == false);
  CHECK(gpsLinkAlive(0xFFFFFF00UL, 0x100UL, 3000) == true); // millis() wrap
}

TEST(gps_fix_text_fits_5_chars) {
  char buf[6];
  gpsFixText(0, buf, sizeof(buf)); CHECK_STR(buf, "NoFix");
  gpsFixText(1, buf, sizeof(buf)); CHECK_STR(buf, "GPS");
  gpsFixText(2, buf, sizeof(buf)); CHECK_STR(buf, "DGPS");
  gpsFixText(6, buf, sizeof(buf)); CHECK_STR(buf, "Q6");
}

TEST(gps_utc_from_gps) {
  astro::UtcTime u;
  CHECK(!utcFromGps(0, 0, 0, 0, 0, 0, 0, u));          // GPS before the first RMC
  CHECK(utcFromGps(26, 11, 22, 13, 5, 7, 250, u));
  CHECK(u.year == 2026 && u.month == 11 && u.day == 22 && u.hour == 13 && u.minute == 5);
  CHECK_NEAR(u.second, 7.25, 1e-12);
}
