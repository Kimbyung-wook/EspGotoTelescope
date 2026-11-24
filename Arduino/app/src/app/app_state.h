#pragma once
#ifndef _APP_STATE_H_
#define _APP_STATE_H_
#include "../gps/gps_data.h"
#include "../sensors/imu_data.h"
#include "../config/settings.h"

// App-wide state shared by the main loop, the UI, the astro glue and the settings:
//   Telemetry : measured and computed values (the main loop fills them every frame)
//   UiState   : values the UI changes (saved settings, run state, requests to the main loop)

/************************************/
// Values the UI shows (filled by the main loop every frame)
/************************************/
// GpsRaw (GPS receiver output): src/gps/gps_data.h

// Position and time in use (src/location, DES-LOC-01, DES-LOC-04), filled every frame
struct ObserverState {
  int    loc_source = 0;                    // loc::Source : NONE / GPS / LAST / MANUAL
  double lat_deg = 0.0, lon_deg = 0.0;
  float  alt_m = 0.0f;
  int    time_source = 0;                   // loc::TimeSource : NONE / GPS / MANUAL / PHONE
  double jd_ut = 0.0;                       // valid when time_source != NONE
  int    clock_hour = 0, clock_minute = 0, clock_second = 0, clock_ms = 0;   // UTC, status bar
};

// A Moon or planet position (src/astro, recomputed about once per second while
// position and time are known)
struct SkyBody {
  bool   valid = false;
  double ra_deg = 0.0, dec_deg = 0.0;       // topocentric, of date
  double ha_deg = 0.0;                      // hour angle, -180..180
  double alt_deg = 0.0, az_deg = 0.0;       // az from north, east positive
  double dist = 0.0;                        // Moon: Earth radii, planets: AU
  double rate_arcsec_s = 0.0;               // RA-axis tracking rate
};

// Where the tube points, from the estimated roll/pitch (polar-aligned equatorial mount)
struct ScopeState {
  bool   valid = false;
  double ra_deg = 0.0, dec_deg = 0.0;
  double tube_alt_deg = 0.0;                // optical axis above the horizon (gravity only)
  // Polar alignment (DES-PA-03): hour angle of Polaris
  bool   polaris_valid = false;
  double polaris_ha_deg = 0.0;
};

struct ImuState {
  ImuData raw;                              // no calibration applied
  ImuData corr;                             // gyro bias removed, accelerometer calibrated
  float roll_acc_deg = 0.0f, pitch_acc_deg = 0.0f;  // tilt from gravity only
  float roll_est_deg = 0.0f, pitch_est_deg = 0.0f;  // complementary filter
};

// Power (src/sensors/battery.h): status bar shows "V --.--" until measured
struct PowerState {
  bool  valid = false;
  float volts = 0.0f;                       // [V] supply, filtered
};

// Loop timing [us]
struct HealthState {
  float loop_hz = 10.0f;
  unsigned long loop_us = 0, main_us = 0, disp_us = 0, gps_us = 0, imu_us = 0, est_us = 0, astro_us = 0;
};

struct Telemetry {
  unsigned long now_ms = 0;
  GpsRaw        gps;
  ObserverState obs;
  SkyBody       moon;                       // also the lunar tracking rate
  // Tracking target chosen on the 1Target menu (index of astro::Target)
  int           target_index = 0;           // which target 'target' belongs to
  SkyBody       target;
  bool          ra_calibrated = false;      // RAduty setting is not 0 (motor can track)
  ScopeState    scope;
  ImuState      imu;
  PowerState    power;

  // Phone remote UI (src/remote): status bar mark in the last column
  enum RemoteState { REMOTE_OFF = 0, REMOTE_WAITING = 1, REMOTE_CONNECTED = 2 };
  int remote = REMOTE_OFF;                  // OFF: no mark, WAITING: 'w', CONNECTED: 'W'

  // Settings in flash (src/config): shown on the Settings screen
  enum SettingsStatus { SETTINGS_DEFAULT = 0, SETTINGS_LOADED, SETTINGS_SAVED, SETTINGS_SAVE_FAILED };
  int settings_status = SETTINGS_DEFAULT;

  HealthState   health;
};

/************************************/
// Values the UI changes (read by the main loop)
/************************************/
struct UiState {
  // Saved settings (DES-CFG): calibration, location, motor, tracking, guidance.
  // The one copy: loaded at boot, saved on a request (app/settings_binding.h).
  config::Settings cfg = config::defaults();

  // Run state, not saved
  float ra_speed_cmd = 0.0f;                // RA motor speed command
  bool  tracking = false;                   // RA motor turns at the tracking rate
  bool  polar_aligned = false;              // confirmed on Polar Align
  // Manual clock (DES-LOC-04): jd at the moment it was set
  bool   manual_clock_valid = false;
  double manual_clock_jd = 0.0;
  unsigned long manual_clock_ms = 0;
  // Phone clock (DES-LOC-05): sent by the remote UI page when it connects
  bool   phone_clock_valid = false;
  double phone_clock_jd = 0.0;
  unsigned long phone_clock_ms = 0;

  // Set again every frame by the screen on top (ScreenManager clears them):
  bool  motor_inhibit = false;              // motor stop (DES-SYS-02), tracking state is kept
  bool  display_off = false;                // Sleep screen

  // Requests to the main loop (app::serviceRequests clears them)
  bool settings_save_request = false;       // write the current values to flash
  bool settings_reset_request = false;      // back to defaults, then save

  // cfg.track_mode values (DES-TRK-01)
  enum TrackMode { TRACK_SIDEREAL = 0, TRACK_LUNAR = 1 };
};

struct UiContext {
  Telemetry tel;
  UiState   state;
};

#endif // _APP_STATE_H_
