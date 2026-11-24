// RA tracking policy (DES-TRK-01, DES-TRK-02, DES-MOT-02): which rate, which
// direction, and the motor duty while tracking is on.
#pragma once
#include "app_state.h"
#include "../astro/astro.h"
#include "../location/location.h"

namespace app {

// RA motor duty for a tracking rate, open loop and linear around the sidereal rate.
// duty_at_sidereal <= 0 means "not calibrated": returns 0 (do not drive).
inline float trackingDuty(double rate_arcsec_s, float duty_at_sidereal) {
  if (duty_at_sidereal <= 0.0f) return 0.0f;
  return (float)(duty_at_sidereal * rate_arcsec_s / astro::SIDEREAL_RATE_ARCSEC_S);
}

// Tracking rate of the chosen mode (DES-TRK-01). Lunar needs the Moon position.
inline bool trackingRate(const Telemetry& t, const UiState& st, double& rate_arcsec_s) {
  if (st.cfg.track_mode == UiState::TRACK_LUNAR) {
    if (!t.moon.valid) return false;
    rate_arcsec_s = t.moon.rate_arcsec_s;
    return true;
  }
  rate_arcsec_s = astro::SIDEREAL_RATE_ARCSEC_S;
  return true;
}

// Direction of the tracking command (DES-MOT-02): -1 in the southern hemisphere,
// flipped again by the motor_reverse setting.
inline float trackingSign(const Telemetry& t, const UiState& st) {
  float s = (t.obs.loc_source != loc::SRC_NONE && t.obs.lat_deg < 0.0) ? -1.0f : 1.0f;
  return st.cfg.motor_reverse ? -s : s;
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
  st.ra_speed_cmd = trackingSign(t, st) * (trackingDuty(rate, duty_at_sidereal) + st.cfg.track_offset);
  return true;
}

}  // namespace app
