#pragma once
#ifndef _APP_SETTINGS_BINDING_H_
#define _APP_SETTINGS_BINDING_H_
#include "app_state.h"
#include "../config/settings.h"

/************************************/
// Saved settings (UiState::cfg) <-> flash
//   boot  : loadSettings()
//   frame : serviceRequests() saves when a screen asked for it (DES-CFG-04)
/************************************/
namespace app {

inline int statusAfterLoad(config::LoadResult r) {
  return r == config::LoadResult::Loaded ? Telemetry::SETTINGS_LOADED : Telemetry::SETTINGS_DEFAULT;
}

// Boot: read the settings into UiState::cfg (defaults on any problem).
inline config::LoadResult loadSettings(config::SettingsStore& store, UiContext& ctx) {
  const config::LoadResult r = config::load(store, ctx.state.cfg);
  ctx.tel.settings_status = statusAfterLoad(r);
  return r;
}

// Call once per frame after the UI step. Returns true when flash was written.
inline bool serviceRequests(config::SettingsStore& store, UiContext& ctx) {
  UiState& st = ctx.state;
  if (st.settings_reset_request) {          // Settings > Reset defaults (confirmed)
    st.settings_reset_request = false;
    st.cfg = config::defaults();
    st.settings_save_request = true;
  }
  if (!st.settings_save_request) return false;
  st.settings_save_request = false;
  const config::SaveResult r = config::save(store, st.cfg);
  ctx.tel.settings_status = r == config::SaveResult::Failed ? Telemetry::SETTINGS_SAVE_FAILED
                                                            : Telemetry::SETTINGS_SAVED;
  return r == config::SaveResult::Saved;
}

} // namespace app

#endif // _APP_SETTINGS_BINDING_H_
