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

  // Position and time in use (src/location, DES-LOC-01, DES-LOC-04), filled every frame
  int    loc_source = 0;                    // loc::Source : NONE / GPS / LAST / MANUAL
  double obs_lat_deg = 0.0, obs_lon_deg = 0.0;
  float  obs_alt_m = 0.0f;
  int    time_source = 0;                   // loc::TimeSource : NONE / GPS / MANUAL
  double jd_ut = 0.0;                       // valid when time_source != NONE
  int    clock_hour = 0, clock_minute = 0, clock_second = 0, clock_ms = 0;   // UTC, status bar

  // Moon (src/astro, recomputed about once per second while position and time are known)
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

  // Where the tube points, from the estimated roll/pitch (polar-aligned equatorial mount)
  bool   scope_valid = false;
  double scope_ra_deg = 0.0, scope_dec_deg = 0.0;
  double tube_alt_deg = 0.0;               // optical axis above the horizon (gravity only)

  // Polar alignment (DES-PA-03): hour angle of Polaris
  bool   polaris_valid = false;
  double polaris_ha_deg = 0.0;

  // IMU
  ImuData imu;                              // raw (no calibration applied)
  ImuData imu_corr;                         // gyro bias removed, accelerometer calibrated
  float roll_acc_deg = 0.0f, pitch_acc_deg = 0.0f;  // tilt from gravity only
  float roll_est_deg = 0.0f, pitch_est_deg = 0.0f;  // complementary filter

  // Power
  float voltage = 12.60f;                   // not measured yet

  // Phone remote UI (src/remote): status bar mark in the last column
  enum RemoteState { REMOTE_OFF = 0, REMOTE_WAITING = 1, REMOTE_CONNECTED = 2 };
  int remote = REMOTE_OFF;                  // OFF: no mark, WAITING: 'w', CONNECTED: 'W'

  // Settings in flash (src/config): shown on the Settings screen
  enum SettingsStatus { SETTINGS_DEFAULT = 0, SETTINGS_LOADED, SETTINGS_SAVED, SETTINGS_SAVE_FAILED };
  int settings_status = SETTINGS_DEFAULT;

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
  float accel_bias[3] = {0.0f, 0.0f, 0.0f}; // [m/s^2] six-position calibration
  float accel_scale[3] = {1.0f, 1.0f, 1.0f};
  int   track_target = 1;                   // astro::Target index (1 = Mercury)
  bool  tracking = false;                   // RA motor turns at the tracking rate
  enum TrackMode { TRACK_SIDEREAL = 0, TRACK_LUNAR = 1 };
  int   track_mode = TRACK_SIDEREAL;        // DES-TRK-01
  float track_offset = 0.0f;                // added to the tracking command [PWM duty]
  float ra_duty_at_sidereal = 0.0f;         // RRcmd at the sidereal rate (from the settings), 0 = not set
  bool  motor_reverse = false;              // wiring: swap the tracking direction (DES-MOT-02)
  float guide_tolerance_deg = 1.0f;         // ON TARGET when both offsets are inside (DES-PNT-04)

  // Motor stop (DES-SYS-02): cleared every frame, set by the calibration and
  // Polar Align screens while they are on top. Tracking state is kept.
  bool  motor_inhibit = false;
  bool  polar_aligned = false;              // confirmed on Polar Align (not saved)

  // Location (DES-LOC-01..03), saved
  int    location_mode = 0;                 // loc::Mode : AUTO / MANUAL
  bool   manual_set = false;                // the manual position was entered
  double manual_lat_deg = 0.0, manual_lon_deg = 0.0;
  float  manual_alt_m = 0.0f;
  bool   last_gps_valid = false;            // last good GPS fix (DES-LOC-02)
  double last_lat_deg = 0.0, last_lon_deg = 0.0;
  float  last_alt_m = 0.0f;
  int    last_year = 0, last_month = 0, last_day = 0;
  int    utc_offset_min = 0;                // local = UTC + this

  // Manual clock (DES-LOC-04), not saved: jd at the moment it was set
  bool   manual_clock_valid = false;
  double manual_clock_jd = 0.0;
  unsigned long manual_clock_ms = 0;

  // Requests to the main loop (config::serviceRequests clears them)
  bool settings_save_request = false;       // write the current values to flash
  bool settings_reset_request = false;      // back to defaults, then save
};

struct UiContext {
  Telemetry tel;
  UiState   state;
};

#endif // _UI_CONTEXT_H_
