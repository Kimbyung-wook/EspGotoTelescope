// Fill the Moon fields of Telemetry from the GPS fields (shared by app.ino and the UI simulator).
#pragma once
#include "moon_tracker.h"
#include "../ui/ui_context.h"

namespace astro {

// Returns true when a new Moon state was computed in this call.
inline bool updateMoonTelemetry(MoonTracker& tracker, Telemetry& t) {
  UtcTime utc;
  const bool date_ok = utcFromGps(t.year, t.month, t.day, t.hour, t.minute, t.seconds,
                                  t.milliseconds, utc);
  const Observer obs{t.latitude, t.longitude, t.altitude_m};
  const bool fresh = tracker.update(t.now_ms, t.fixquality > 0, date_ok ? &utc : nullptr, obs);
  t.moon_valid = tracker.valid();
  if (t.moon_valid) {
    const MoonState& s = tracker.state();
    t.moon_ra_deg = s.topo.ra_deg;
    t.moon_dec_deg = s.topo.dec_deg;
    t.moon_ha_deg = s.ha_deg;
    t.moon_alt_deg = s.hor.alt_deg;
    t.moon_az_deg = s.hor.az_deg;
    t.moon_rate_arcsec_s = s.ha_rate_arcsec_s;
  }
  return fresh;
}

}  // namespace astro
