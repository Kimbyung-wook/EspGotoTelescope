// Fill the Moon / tracking-target / tube fields of Telemetry from the position and
// time in use (Telemetry::obs; see loop_logic.h). Shared by app.ino and
// the UI simulator.
#pragma once
#include "app_state.h"
#include "../astro/target_tracker.h"
#include "../location/location.h"
#include "../util/angles.h"
#include <math.h>

namespace app {

namespace detail {
inline bool known(const Telemetry& t) { return t.obs.loc_source != loc::SRC_NONE && t.obs.time_source != loc::TIME_NONE; }
inline astro::Observer observer(const Telemetry& t) { return astro::Observer{t.obs.lat_deg, t.obs.lon_deg, t.obs.alt_m}; }
inline bool trackerUpdate(astro::TargetTracker& tr, const Telemetry& t) {
  return tr.updateJd(t.now_ms, known(t), t.obs.jd_ut, observer(t));
}
// One tracker result into a SkyBody (keeps the last values while it is invalid)
inline bool fillBody(const astro::TargetTracker& tracker, SkyBody& b) {
  b.valid = tracker.valid();
  if (!b.valid) return false;
  const astro::TargetState& s = tracker.state();
  b.ra_deg = s.topo.ra_deg;
  b.dec_deg = s.topo.dec_deg;
  b.ha_deg = s.ha_deg;
  b.alt_deg = s.hor.alt_deg;
  b.az_deg = s.hor.az_deg;
  b.dist = s.topo.dist;
  b.rate_arcsec_s = s.ha_rate_arcsec_s;
  return true;
}
}  // namespace detail

// Moon (also the lunar tracking rate). Returns true when a new state was computed.
inline bool updateMoonTelemetry(astro::TargetTracker& tracker, Telemetry& t) {
  tracker.setTarget(astro::Target::Moon);
  const bool fresh = detail::trackerUpdate(tracker, t);
  detail::fillBody(tracker, t.moon);
  return fresh;
}

// Target screen: the target chosen in UiState::cfg.track_target.
inline bool updateTargetTelemetry(astro::TargetTracker& tracker, int target_index, Telemetry& t) {
  if (target_index < 0 || target_index >= astro::TARGET_COUNT) target_index = 0;
  tracker.setTarget(static_cast<astro::Target>(target_index));
  const bool fresh = detail::trackerUpdate(tracker, t);
  t.target_index = target_index;
  detail::fillBody(tracker, t.target);
  return fresh;
}

// Tube altitude (always), telescope RA/Dec and the Polaris hour angle (when the
// position and time are known), every frame (a few trig calls).
// The mirror ambiguity is resolved toward the chosen target.
inline bool updateScopeTelemetry(Telemetry& t, int optical_axis, int dec_axis) {
  double up[3];
  astro::upInTubeFromRollPitch(t.imu.roll_est_deg, t.imu.pitch_est_deg, optical_axis, dec_axis, up);
  const double u0 = up[0] > 1.0 ? 1.0 : (up[0] < -1.0 ? -1.0 : up[0]);
  t.scope.tube_alt_deg = asin(u0) * util::kRadToDeg;
  t.scope.valid = false;
  t.scope.polaris_valid = false;
  if (!detail::known(t)) return false;
  const double lst = astro::lstDeg(t.obs.jd_ut, t.obs.lon_deg);
  t.scope.polaris_ha_deg = astro::polarisHaDeg(t.obs.jd_ut, t.obs.lon_deg);
  t.scope.polaris_valid = t.obs.lat_deg > 0.0;            // Polaris: northern sky only
  const double hint_ra = t.target.valid ? t.target.ra_deg : lst;          // else: the meridian
  const double hint_dec = t.target.valid ? t.target.dec_deg : t.obs.lat_deg - 45.0;
  astro::Equatorial q;
  if (!astro::pointingFromUp(up, t.obs.lat_deg, lst, hint_ra, hint_dec, q)) return false;
  t.scope.ra_deg = q.ra_deg;
  t.scope.dec_deg = q.dec_deg;
  t.scope.valid = true;
  return true;
}

}  // namespace app
