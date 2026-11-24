// Fill the Moon / tracking-target fields of Telemetry from the GPS fields
// (shared by app.ino and the UI simulator).
#pragma once
#include "target_tracker.h"
#include "../ui/ui_context.h"
#include <math.h>

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

// While tracking is on, drive the RA motor at the target rate plus the user offset
// (Planet Tracking > Offset). Before calibration (duty_at_sidereal 0) only the
// offset drives the motor, so the offset can be used to find the sidereal duty.
// Keeps the last command if the GPS drops out for a moment.
inline bool applyTracking(const Telemetry& t, UiState& st, float duty_at_sidereal) {
  if (!st.tracking) return false;
  if (!t.target_valid || t.target_index != st.track_target) return false;
  st.ra_speed_cmd = trackingDuty(t.target_rate_arcsec_s, duty_at_sidereal) + st.track_offset;
  return true;
}

// "Up" in the tube frame from roll/pitch (rollFromAccelDeg / pitchFromAccelDeg
// conventions) and the IMU axes along the optical axis and the Dec axis.
inline void upInTubeFromRollPitch(double roll_deg, double pitch_deg, int optical_axis, int dec_axis,
                                  double up[3]) {
  const double D2R = 3.14159265358979323846 / 180.0;
  const double r = roll_deg * D2R, p = pitch_deg * D2R;
  const double ub[3] = {-sin(p), cos(p) * sin(r), cos(p) * cos(r)};   // ~ accelerometer / g
  auto axis = [](int a, double v[3]) {
    v[0] = v[1] = v[2] = 0.0;
    const int i = (a < 0 ? -a : a) - 1;
    if (i >= 0 && i < 3) v[i] = a < 0 ? -1.0 : 1.0;
  };
  double o[3], d[3];
  axis(optical_axis, o);
  axis(dec_axis, d);
  const double z[3] = {o[1] * d[2] - o[2] * d[1], o[2] * d[0] - o[0] * d[2], o[0] * d[1] - o[1] * d[0]};
  up[0] = ub[0] * o[0] + ub[1] * o[1] + ub[2] * o[2];
  up[1] = ub[0] * d[0] + ub[1] * d[1] + ub[2] * d[2];
  up[2] = ub[0] * z[0] + ub[1] * z[1] + ub[2] * z[2];
}

// Telescope RA/Dec from the estimated roll/pitch, every frame (a few trig calls).
// The mirror ambiguity is resolved toward the chosen target.
inline bool updateScopeTelemetry(Telemetry& t, int optical_axis, int dec_axis) {
  UtcTime utc;
  t.scope_valid = false;
  if (t.fixquality <= 0 ||
      !utcFromGps(t.year, t.month, t.day, t.hour, t.minute, t.seconds, t.milliseconds, utc))
    return false;
  double up[3];
  upInTubeFromRollPitch(t.roll_est_deg, t.pitch_est_deg, optical_axis, dec_axis, up);
  const double lst = lstDeg(julianDate(utc), t.longitude);
  const double hint_ra = t.target_valid ? t.target_ra_deg : lst;          // else: the meridian
  const double hint_dec = t.target_valid ? t.target_dec_deg : t.latitude - 45.0;
  Equatorial q;
  if (!pointingFromUp(up, t.latitude, lst, hint_ra, hint_dec, q)) return false;
  t.scope_ra_deg = q.ra_deg;
  t.scope_dec_deg = q.dec_deg;
  t.scope_valid = true;
  return true;
}

}  // namespace astro
