#pragma once
#ifndef _CONFIG_SETTINGS_BINDING_H_
#define _CONFIG_SETTINGS_BINDING_H_
#include "settings.h"
#include "../ui/ui_context.h"

/************************************/
// Settings <-> the values the UI and the main loop use (UiState)
//   boot  : load() -> applyToUi()
//   frame : serviceRequests() saves when a screen asked for it (DES-CFG-04)
// The magnetometer fields (mag_offset, mag_scale, declination_deg) are not in
// UiState yet (DES-MAG): they are kept as loaded.
/************************************/
namespace config {

inline void applyToUi(const Settings& s, UiState& st) {
  for (int i = 0; i < 3; i++) {
    st.gyro_bias[i]   = s.gyro_bias[i];
    st.accel_bias[i]  = s.accel_bias[i];
    st.accel_scale[i] = s.accel_scale[i];
  }
  st.location_mode  = s.location_mode;
  st.manual_set     = s.manual_set != 0;
  st.manual_lat_deg = s.manual_lat_deg;
  st.manual_lon_deg = s.manual_lon_deg;
  st.manual_alt_m   = s.manual_alt_m;
  st.last_gps_valid = s.last_gps_valid != 0;
  st.last_lat_deg   = s.last_lat_deg;
  st.last_lon_deg   = s.last_lon_deg;
  st.last_alt_m     = s.last_alt_m;
  st.last_year      = s.last_year;
  st.last_month     = s.last_month;
  st.last_day       = s.last_day;
  st.utc_offset_min = s.utc_offset_min;
  st.ra_duty_at_sidereal = s.ra_duty_at_sidereal;
  st.motor_reverse  = s.motor_reverse != 0;
  st.track_mode     = s.track_mode;
  st.track_offset   = s.track_offset;
  st.track_target   = s.track_target;
  st.guide_tolerance_deg = s.guide_tolerance_deg;
}

inline void collectFromUi(const UiState& st, Settings& s) {
  for (int i = 0; i < 3; i++) {
    s.gyro_bias[i]   = st.gyro_bias[i];
    s.accel_bias[i]  = st.accel_bias[i];
    s.accel_scale[i] = st.accel_scale[i];
  }
  s.location_mode  = (uint8_t)st.location_mode;
  s.manual_set     = st.manual_set ? 1 : 0;
  s.manual_lat_deg = st.manual_lat_deg;
  s.manual_lon_deg = st.manual_lon_deg;
  s.manual_alt_m   = st.manual_alt_m;
  s.last_gps_valid = st.last_gps_valid ? 1 : 0;
  s.last_lat_deg   = st.last_lat_deg;
  s.last_lon_deg   = st.last_lon_deg;
  s.last_alt_m     = st.last_alt_m;
  s.last_year      = (uint16_t)st.last_year;
  s.last_month     = (uint8_t)st.last_month;
  s.last_day       = (uint8_t)st.last_day;
  s.utc_offset_min = (int16_t)st.utc_offset_min;
  s.ra_duty_at_sidereal = st.ra_duty_at_sidereal;
  s.motor_reverse  = st.motor_reverse ? 1 : 0;
  s.track_mode     = (uint8_t)st.track_mode;
  s.track_offset   = st.track_offset;
  s.track_target   = (int8_t)st.track_target;
  s.guide_tolerance_deg = st.guide_tolerance_deg;
}

inline int statusAfterLoad(LoadResult r) {
  return r == LoadResult::Loaded ? Telemetry::SETTINGS_LOADED : Telemetry::SETTINGS_DEFAULT;
}

// Call once per frame after the UI step. Returns true when flash was written.
inline bool serviceRequests(SettingsStore& store, Settings& s, UiContext& ctx) {
  UiState& st = ctx.state;
  if (st.settings_reset_request) {          // Settings > Reset defaults (confirmed)
    st.settings_reset_request = false;
    s = defaults();
    applyToUi(s, st);
    st.settings_save_request = true;
  }
  if (!st.settings_save_request) return false;
  st.settings_save_request = false;
  collectFromUi(st, s);
  const SaveResult r = save(store, s);
  ctx.tel.settings_status = r == SaveResult::Failed ? Telemetry::SETTINGS_SAVE_FAILED
                                                    : Telemetry::SETTINGS_SAVED;
  return r == SaveResult::Saved;
}

} // namespace config

#endif // _CONFIG_SETTINGS_BINDING_H_
