#pragma once
#ifndef _UI_CONTEXT_H_
#define _UI_CONTEXT_H_
#include "../sensors/imu_data.h"

/************************************/
// Values the UI shows (filled by the main loop every frame)
/************************************/
struct Telemetry {
  unsigned long now_ms = 0;

  // GPS
  int  year = 0, month = 0, day = 0;
  int  hour = 0, minute = 0, seconds = 0, milliseconds = 0;
  int  fixquality = 0, satellites = 0;
  char lat_hemi = ' ', lon_hemi = ' ';
  double latitude = 0.0, longitude = 0.0;   // [deg], signed (S/W negative)
  float  altitude_m = 0.0f;                 // above mean sea level

  // Moon (src/astro, recomputed about once per second while the GPS has a fix)
  bool   moon_valid = false;
  double moon_ra_deg = 0.0, moon_dec_deg = 0.0;   // topocentric, of date
  double moon_ha_deg = 0.0;                       // hour angle, -180..180
  double moon_alt_deg = 0.0, moon_az_deg = 0.0;   // az from north, east positive
  double moon_rate_arcsec_s = 0.0;                // RA-axis tracking rate

  // Tracking target chosen on the Planet Tracking screen (index of astro::Target)
  int    target_index = 0;                 // which target the fields below belong to
  bool   target_valid = false;
  double target_ra_deg = 0.0, target_dec_deg = 0.0, target_ha_deg = 0.0;
  double target_alt_deg = 0.0, target_az_deg = 0.0;
  double target_dist = 0.0;                // Moon: Earth radii, planets: AU
  double target_rate_arcsec_s = 0.0;
  bool   ra_calibrated = false;            // RA_DUTY_AT_SIDEREAL is set (motor can track)

  // IMU
  ImuData imu;                              // raw (gyro bias not removed)
  ImuData imu_corr;                         // gyro bias removed
  float roll_acc_deg = 0.0f, pitch_acc_deg = 0.0f;  // tilt from gravity only
  float roll_est_deg = 0.0f, pitch_est_deg = 0.0f;  // complementary filter

  // Power
  float voltage = 12.60f;                   // not measured yet

  // Health [us]
  float loop_hz = 10.0f;
  unsigned long loop_us = 0, main_us = 0, disp_us = 0, gps_us = 0, imu_us = 0, est_us = 0, astro_us = 0;
};

/************************************/
// Values the UI changes (read by the main loop)
/************************************/
struct UiState {
  float ra_speed_cmd = 0.0f;                // RA motor speed command
  float gyro_bias[3] = {0.0f, 0.0f, 0.0f};  // [deg/s]
  float accel_bias[3] = {0.0f, 0.0f, 0.0f}; // [m/s^2] (calibration not implemented)
  int   track_target = 1;                   // astro::Target index (1 = Mercury)
  bool  tracking = false;                   // RA motor follows the target rate
};

struct UiContext {
  Telemetry tel;
  UiState   state;
};

#endif // _UI_CONTEXT_H_
