// Fill the Moon / tracking-target / tube fields of Telemetry from the position and
// time in use (Telemetry::obs_*, jd_ut; see src/app/loop_logic.h), and the RA motor
// command while tracking. Shared by app.ino and the UI simulator.
#pragma once
#include "target_tracker.h"
#include "../ui/ui_context.h"
#include <math.h>

namespace astro {

namespace detail {
inline bool known(const Telemetry& t) { return t.loc_source != 0 && t.time_source != 0; }
inline Observer observer(const Telemetry& t) { return Observer{t.obs_lat_deg, t.obs_lon_deg, t.obs_alt_m}; }
inline bool trackerUpdate(TargetTracker& tr, const Telemetry& t) {
  return tr.updateJd(t.now_ms, known(t), t.jd_ut, observer(t));
}
}  // namespace detail

// Moon (also the lunar tracking rate). Returns true when a new state was computed.
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

// Target screen: the target chosen in UiState::track_target.
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

// Tracking rate of the chosen mode (DES-TRK-01). Lunar needs the Moon position.
inline bool trackingRate(const Telemetry& t, const UiState& st, double& rate_arcsec_s) {
  if (st.track_mode == UiState::TRACK_LUNAR) {
    if (!t.moon_valid) return false;
    rate_arcsec_s = t.moon_rate_arcsec_s;
    return true;
  }
  rate_arcsec_s = SIDEREAL_RATE_ARCSEC_S;
  return true;
}

// Direction of the tracking command (DES-MOT-02): -1 in the southern hemisphere,
// flipped again by the motor_reverse setting.
inline float trackingSign(const Telemetry& t, const UiState& st) {
  float s = (t.loc_source != 0 && t.obs_lat_deg < 0.0) ? -1.0f : 1.0f;
  return st.motor_reverse ? -s : s;
}

// While tracking is on, drive the RA motor at the tracking rate plus the user
// offset (Target > Offset). The tube itself is followed (DES-TRK-02), so the
// sidereal rate works without a position. Before calibration (duty_at_sidereal 0)
// only the offset drives the motor, so the offset can be used to find the sidereal
// duty. Keeps the last command if the lunar rate is missing for a moment.
inline bool applyTracking(const Telemetry& t, UiState& st, float duty_at_sidereal) {
  if (!st.tracking) return false;
  double rate = 0.0;
  if (!trackingRate(t, st, rate)) return false;
  st.ra_speed_cmd = trackingSign(t, st) * (trackingDuty(rate, duty_at_sidereal) + st.track_offset);
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

// Tube altitude (always), telescope RA/Dec and the Polaris hour angle (when the
// position and time are known), every frame (a few trig calls).
// The mirror ambiguity is resolved toward the chosen target.
inline bool updateScopeTelemetry(Telemetry& t, int optical_axis, int dec_axis) {
  double up[3];
  upInTubeFromRollPitch(t.roll_est_deg, t.pitch_est_deg, optical_axis, dec_axis, up);
  const double u0 = up[0] > 1.0 ? 1.0 : (up[0] < -1.0 ? -1.0 : up[0]);
  t.tube_alt_deg = asin(u0) * 57.29577951308232;
  t.scope_valid = false;
  t.polaris_valid = false;
  if (!detail::known(t)) return false;
  const double lst = lstDeg(t.jd_ut, t.obs_lon_deg);
  t.polaris_ha_deg = polarisHaDeg(t.jd_ut, t.obs_lon_deg);
  t.polaris_valid = t.obs_lat_deg > 0.0;            // Polaris: northern sky only
  const double hint_ra = t.target_valid ? t.target_ra_deg : lst;          // else: the meridian
  const double hint_dec = t.target_valid ? t.target_dec_deg : t.obs_lat_deg - 45.0;
  Equatorial q;
  if (!pointingFromUp(up, t.obs_lat_deg, lst, hint_ra, hint_dec, q)) return false;
  t.scope_ra_deg = q.ra_deg;
  t.scope_dec_deg = q.dec_deg;
  t.scope_valid = true;
  return true;
}

}  // namespace astro
