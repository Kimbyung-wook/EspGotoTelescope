// Fill the Moon / tracking-target / tube fields of Telemetry from the position and
// time in use (Telemetry::obs_*, jd_ut; see loop_logic.h). Shared by app.ino and
// the UI simulator.
#pragma once
#include "app_state.h"
#include "../astro/target_tracker.h"
#include "../location/location.h"
#include "../util/angles.h"
#include <math.h>

namespace app {

namespace detail {
inline bool known(const Telemetry& t) { return t.loc_source != loc::SRC_NONE && t.time_source != loc::TIME_NONE; }
inline astro::Observer observer(const Telemetry& t) { return astro::Observer{t.obs_lat_deg, t.obs_lon_deg, t.obs_alt_m}; }
inline bool trackerUpdate(astro::TargetTracker& tr, const Telemetry& t) {
  return tr.updateJd(t.now_ms, known(t), t.jd_ut, observer(t));
}
}  // namespace detail

// Moon (also the lunar tracking rate). Returns true when a new state was computed.
inline bool updateMoonTelemetry(astro::TargetTracker& tracker, Telemetry& t) {
  tracker.setTarget(astro::Target::Moon);
  const bool fresh = detail::trackerUpdate(tracker, t);
  t.moon_valid = tracker.valid();
  if (t.moon_valid) {
    const astro::TargetState& s = tracker.state();
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
inline bool updateTargetTelemetry(astro::TargetTracker& tracker, int target_index, Telemetry& t) {
  if (target_index < 0 || target_index >= astro::TARGET_COUNT) target_index = 0;
  tracker.setTarget(static_cast<astro::Target>(target_index));
  const bool fresh = detail::trackerUpdate(tracker, t);
  t.target_index = target_index;
  t.target_valid = tracker.valid();
  if (t.target_valid) {
    const astro::TargetState& s = tracker.state();
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

// Tube altitude (always), telescope RA/Dec and the Polaris hour angle (when the
// position and time are known), every frame (a few trig calls).
// The mirror ambiguity is resolved toward the chosen target.
inline bool updateScopeTelemetry(Telemetry& t, int optical_axis, int dec_axis) {
  double up[3];
  astro::upInTubeFromRollPitch(t.roll_est_deg, t.pitch_est_deg, optical_axis, dec_axis, up);
  const double u0 = up[0] > 1.0 ? 1.0 : (up[0] < -1.0 ? -1.0 : up[0]);
  t.tube_alt_deg = asin(u0) * util::kRadToDeg;
  t.scope_valid = false;
  t.polaris_valid = false;
  if (!detail::known(t)) return false;
  const double lst = astro::lstDeg(t.jd_ut, t.obs_lon_deg);
  t.polaris_ha_deg = astro::polarisHaDeg(t.jd_ut, t.obs_lon_deg);
  t.polaris_valid = t.obs_lat_deg > 0.0;            // Polaris: northern sky only
  const double hint_ra = t.target_valid ? t.target_ra_deg : lst;          // else: the meridian
  const double hint_dec = t.target_valid ? t.target_dec_deg : t.obs_lat_deg - 45.0;
  astro::Equatorial q;
  if (!astro::pointingFromUp(up, t.obs_lat_deg, lst, hint_ra, hint_dec, q)) return false;
  t.scope_ra_deg = q.ra_deg;
  t.scope_dec_deg = q.dec_deg;
  t.scope_valid = true;
  return true;
}

}  // namespace app
