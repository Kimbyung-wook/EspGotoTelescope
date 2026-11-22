// Fill the Moon / tracking-target fields of Telemetry from the GPS fields
// (shared by app.ino and the UI simulator).
#pragma once
#include "target_tracker.h"
#include "../ui/ui_context.h"

namespace astro {

namespace detail {
inline bool trackerUpdate(TargetTracker& tr, const Telemetry& t) {
  UtcTime utc;
  const bool date_ok = utcFromGps(t.year, t.month, t.day, t.hour, t.minute, t.seconds,
                                  t.milliseconds, utc);
  const Observer obs{t.latitude, t.longitude, t.altitude_m};
  return tr.update(t.now_ms, t.fixquality > 0, date_ok ? &utc : nullptr, obs);
}
}  // namespace detail

// Moon screen. Returns true when a new state was computed in this call.
inline bool updateMoonTelemetry(TargetTracker& tracker, Telemetry& t) {
  tracker.setTarget(Target::Moon);
  const bool fresh = detail::trackerUpdate(tracker, t);
  t.moon_valid = tracker.valid();
  if (t.moon_valid) {
    const TargetState& s = tracker.state();
    t.moon_ra_deg = s.topo.ra_deg;
    t.moon_dec_deg = s.topo.dec_deg;
    t.moon_ha_deg = s.ha_deg;
    t.moon_alt_deg = s.hor.alt_deg;
    t.moon_az_deg = s.hor.az_deg;
    t.moon_rate_arcsec_s = s.ha_rate_arcsec_s;
  }
  return fresh;
}

// Planet Tracking screen: the target chosen in UiState::track_target.
inline bool updateTargetTelemetry(TargetTracker& tracker, int target_index, Telemetry& t) {
  if (target_index < 0 || target_index >= TARGET_COUNT) target_index = 0;
  tracker.setTarget(static_cast<Target>(target_index));
  const bool fresh = detail::trackerUpdate(tracker, t);
  t.target_index = target_index;
  t.target_valid = tracker.valid();
  if (t.target_valid) {
    const TargetState& s = tracker.state();
    t.target_ra_deg = s.topo.ra_deg;
    t.target_dec_deg = s.topo.dec_deg;
    t.target_ha_deg = s.ha_deg;
    t.target_alt_deg = s.hor.alt_deg;
    t.target_az_deg = s.hor.az_deg;
    t.target_dist = s.topo.dist;
    t.target_rate_arcsec_s = s.ha_rate_arcsec_s;
  }
  return fresh;
}

// RA motor duty for a tracking rate, open loop and linear around the sidereal rate.
// duty_at_sidereal <= 0 means "not calibrated": returns 0 (do not drive).
inline float trackingDuty(double rate_arcsec_s, float duty_at_sidereal) {
  if (duty_at_sidereal <= 0.0f) return 0.0f;
  return (float)(duty_at_sidereal * rate_arcsec_s / SIDEREAL_RATE_ARCSEC_S);
}

// While tracking is on, drive the RA motor at the target rate. Keeps the last
// command if the GPS drops out for a moment. Returns true if it set the command.
inline bool applyTracking(const Telemetry& t, UiState& st, float duty_at_sidereal) {
  if (!st.tracking || duty_at_sidereal <= 0.0f) return false;
  if (!t.target_valid || t.target_index != st.track_target) return false;
  st.ra_speed_cmd = trackingDuty(t.target_rate_arcsec_s, duty_at_sidereal);
  return true;
}

}  // namespace astro
