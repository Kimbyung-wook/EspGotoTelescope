#pragma once
#ifndef _CONFIG_SETTINGS_BINDING_H_
#define _CONFIG_SETTINGS_BINDING_H_
#include "settings.h"
#include "../ui/ui_context.h"

/************************************/
// Settings <-> the values the UI and the main loop use (UiState)
//   boot  : load() -> applyToUi()
//   frame : serviceRequests() saves when a screen asked for it (DES-CFG-04)
// Only the values that exist in UiState today are copied; the other fields
// (location, magnetometer, ...) are kept as loaded until their features use them.
/************************************/
namespace config {

inline void applyToUi(const Settings& s, UiState& st) {
  for (int i = 0; i < 3; i++) {
    st.gyro_bias[i]  = s.gyro_bias[i];
    st.accel_bias[i] = s.accel_bias[i];
  }
  st.ra_duty_at_sidereal = s.ra_duty_at_sidereal;
  st.track_offset = s.track_offset;
  st.track_target = s.track_target;
}

inline void collectFromUi(const UiState& st, Settings& s) {
  for (int i = 0; i < 3; i++) {
    s.gyro_bias[i]  = st.gyro_bias[i];
    s.accel_bias[i] = st.accel_bias[i];
  }
  s.ra_duty_at_sidereal = st.ra_duty_at_sidereal;
  s.track_offset = st.track_offset;
  s.track_target = (int8_t)st.track_target;
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
