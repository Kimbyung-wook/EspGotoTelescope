// Per-frame steps of the main loop that do not touch hardware, shared by app.ino
// and the UI simulator (tools/ui_simulator/runner/firmware_ui.cpp) so both run
// the same logic:
//   updateTelemetry (sensors -> Telemetry raw fields)
//   correctImu -> attitude estimator -> updateLocationTime -> updateAstro
//   -> SimpleGoToUi::step -> config::serviceRequests -> motorCommand
#pragma once
#include "app_state.h"
#include "sky.h"
#include "tracking.h"
#include "../estimation/calibration.h"
#include "../location/location.h"
#include "../gps/gps_link.h"

namespace app {

// Calibration applied to the raw IMU sample (DES-ATT-01): gyro bias, accelerometer
// bias and scale; tilt from gravity uses the corrected accelerometer.
inline void correctImu(Telemetry& t, const UiState& st) {
  t.imu_corr = cal::withAccelCalibration(withoutGyroBias(t.imu, st.gyro_bias),
                                         st.accel_bias, st.accel_scale);
  t.roll_acc_deg = rollFromAccelDeg(t.imu_corr);
  t.pitch_acc_deg = pitchFromAccelDeg(t.imu_corr);
}

// A clock set to 'jd' at millis() 'set_ms', running with millis(): jd_ut and the
// status bar fields
inline void runClock(Telemetry& t, double jd, unsigned long set_ms) {
  t.jd_ut = jd + (double)(unsigned long)(t.now_ms - set_ms) / 86400000.0;
  astro::UtcTime m;
  astro::utcFromJd(t.jd_ut + 0.0005 / 86400.0, m);   // round to the millisecond (jd keeps ~10 us)
  t.clock_hour = m.hour; t.clock_minute = m.minute;
  t.clock_second = (int)m.second;
  t.clock_ms = (int)((m.second - (int)m.second) * 1000.0);
}

// Phone time from the remote UI page (DES-LOC-05): Unix seconds and milliseconds.
// Times before 2020 are ignored (a phone with no clock set).
inline void setPhoneClock(UiState& st, unsigned long now_ms, unsigned long unix_s, int ms) {
  if (unix_s < 1577836800UL || ms < 0 || ms > 999) return;   // 2020-01-01
  st.phone_clock_valid = true;
  st.phone_clock_jd = 2440587.5 + ((double)unix_s + ms / 1000.0) / 86400.0;
  st.phone_clock_ms = now_ms;
}

// Position and time in use (DES-LOC-01, DES-LOC-04, DES-LOC-05) from the raw GPS fields of
// Telemetry and the settings in UiState; keeps the last good fix (DES-LOC-02,
// asks the main loop to save it).
inline void updateLocationTime(Telemetry& t, UiState& st, loc::LastGpsRecorder& last_rec) {
  astro::UtcTime u;
  const bool fix = t.fixquality > 0;
  const bool gps_date = utcFromGps(t.year, t.month, t.day, t.hour, t.minute, t.seconds,
                                   t.milliseconds, u);
  loc::Position gps, last, manual, used;
  gps.lat_deg = t.latitude;          gps.lon_deg = t.longitude;          gps.alt_m = t.altitude_m;
  last.lat_deg = st.last_lat_deg;    last.lon_deg = st.last_lon_deg;    last.alt_m = st.last_alt_m;
  manual.lat_deg = st.manual_lat_deg; manual.lon_deg = st.manual_lon_deg; manual.alt_m = st.manual_alt_m;

  if (last_rec.update(t.now_ms, t.fixquality, t.satellites, gps, st.last_gps_valid, last)) {
    st.last_gps_valid = true;
    st.last_lat_deg = gps.lat_deg;
    st.last_lon_deg = gps.lon_deg;
    st.last_alt_m = gps.alt_m;
    st.last_year = gps_date ? u.year : 0;
    st.last_month = gps_date ? u.month : 0;
    st.last_day = gps_date ? u.day : 0;
    st.settings_save_request = true;
    last = gps;
  }

  t.loc_source = loc::resolvePosition(st.location_mode, fix, gps, st.last_gps_valid, last,
                                      st.manual_set, manual, used);
  if (t.loc_source != loc::SRC_NONE) {
    t.obs_lat_deg = used.lat_deg;
    t.obs_lon_deg = used.lon_deg;
    t.obs_alt_m = used.alt_m;
  }

  if (fix && gps_date) {
    t.time_source = loc::TIME_GPS;
    t.jd_ut = astro::julianDate(u);
    t.clock_hour = t.hour; t.clock_minute = t.minute;
    t.clock_second = t.seconds; t.clock_ms = t.milliseconds;
  } else if (st.phone_clock_valid) {      // network time of the phone: before a hand-set clock
    t.time_source = loc::TIME_PHONE;
    runClock(t, st.phone_clock_jd, st.phone_clock_ms);
  } else if (st.manual_clock_valid) {
    t.time_source = loc::TIME_MANUAL;
    runClock(t, st.manual_clock_jd, st.manual_clock_ms);
  } else {
    t.time_source = loc::TIME_NONE;   // status bar keeps the GPS clock fields
    t.clock_hour = t.hour; t.clock_minute = t.minute;
    t.clock_second = t.seconds; t.clock_ms = t.milliseconds;
  }
}

// Moon, target and tube. Returns true when a 1 s astro update ran this frame.
inline bool updateAstro(astro::TargetTracker& moon, astro::TargetTracker& target, Telemetry& t,
                        const UiState& st, int optical_axis, int dec_axis) {
  bool ran = updateMoonTelemetry(moon, t);
  ran |= updateTargetTelemetry(target, st.track_target, t);
  t.ra_calibrated = st.ra_duty_at_sidereal != 0.0f;
  updateScopeTelemetry(t, optical_axis, dec_axis);
  return ran;
}

// RA motor command after the UI step: tracking overrides RRcmd while it is on;
// a calibration or Polar Align screen stops the motor (DES-SYS-02) without
// changing RRcmd or the tracking state.
inline float motorCommand(const Telemetry& t, UiState& st) {
  applyTracking(t, st, st.ra_duty_at_sidereal);
  return st.motor_inhibit ? 0.0f : st.ra_speed_cmd;
}

}  // namespace app
