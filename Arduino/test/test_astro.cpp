// Accuracy tests for src/astro against published worked examples.
//   Meeus, "Astronomical Algorithms" 2nd ed.: 7.a (JD), 12.a/12.b (GMST),
//   25.a (Sun), 33.a (Venus), 47.a (Moon).
#include "test_main.h"
#include "../app/src/astro/astro.h"

using namespace astro;

static const double ARCMIN = 1.0 / 60.0;
static const double EARTH_RADIUS_KM = 6378.14;

TEST(astro_julian_date) {
  CHECK_NEAR(julianDate({2000, 1, 1, 12, 0, 0.0}), 2451545.0, 1e-9);
  CHECK_NEAR(julianDate({1957, 10, 4, 19, 26, 24.0}), 2436116.31, 1e-6);  // Meeus 7.a
  CHECK_NEAR(julianDate({1999, 1, 1, 0, 0, 0.0}), 2451179.5, 1e-9);
  CHECK_NEAR(julianDate({2026, 3, 1, 0, 0, 0.0}) - julianDate({2026, 2, 28, 0, 0, 0.0}), 1.0, 1e-9);
  CHECK_NEAR(julianDate({2024, 3, 1, 0, 0, 0.0}) - julianDate({2024, 2, 28, 0, 0, 0.0}), 2.0, 1e-9);  // leap year
  // millisecond resolution survives in double near JD 2.46e6
  const double a = julianDate({2026, 11, 22, 12, 0, 0.000});
  const double b = julianDate({2026, 11, 22, 12, 0, 0.001});
  CHECK_NEAR((b - a) * 86400.0, 0.001, 1e-5);
}

TEST(astro_wrap) {
  CHECK_NEAR(wrap360(-10.0), 350.0, 1e-12);
  CHECK_NEAR(wrap360(725.0), 5.0, 1e-12);
  CHECK_NEAR(wrap180(190.0), -170.0, 1e-12);
  CHECK_NEAR(wrap180(-190.0), 170.0, 1e-12);
}

TEST(astro_gmst_meeus_12a_12b) {
  CHECK_NEAR(gmstDeg(2446895.5), 197.693195, 1e-5);        // 13h10m46.3668s
  CHECK_NEAR(gmstDeg(2446896.30625), 128.7378734, 1e-5);   // 8h34m57.0896s
  CHECK_NEAR(lstDeg(2446895.5, -77.0), 120.693195, 1e-5);
}

TEST(astro_sun_meeus_25a) {
  const Equatorial s = sunEquatorial(2448908.5);  // 1992-10-13 0h TD
  CHECK_NEAR(s.ra_deg, 198.38083, 2 * ARCMIN);
  CHECK_NEAR(s.dec_deg, -7.78507, 2 * ARCMIN);
  CHECK_NEAR(s.dist, 0.99766, 0.0002);
}

TEST(astro_moon_meeus_47a) {
  const Equatorial m = moonGeocentric(2448724.5);  // 1992-04-12 0h TD
  CHECK_NEAR(m.ra_deg, 134.688470, 3 * ARCMIN);
  CHECK_NEAR(m.dec_deg, 13.768368, 3 * ARCMIN);
  CHECK_NEAR(m.dist * EARTH_RADIUS_KM, 368409.7, 600.0);
}

TEST(astro_venus_meeus_33a) {
  // Meeus gives apparent of date; ours is J2000 geometric, so allow ~10'
  const Equatorial v = planetGeocentric(Planet::Venus, 2448976.5);  // 1992-12-20 0h TD
  CHECK_NEAR(v.ra_deg, 316.172725, 10 * ARCMIN);
  CHECK_NEAR(v.dec_deg, -18.888011, 10 * ARCMIN);
  CHECK_NEAR(v.dist, 0.910947, 0.001);
}

TEST(astro_mars_opposition_2003) {
  // Closest approach 2003-08-27 ~9:51 UT, 0.37272 AU
  const double jd = ttFromUt(julianDate({2003, 8, 27, 12, 0, 0.0}));
  CHECK_NEAR(planetGeocentric(Planet::Mars, jd).dist, 0.3727, 0.002);
}

TEST(astro_topocentric_overhead_and_horizon) {
  Observer eq{0.0, 0.0};
  Equatorial geo{100.0, 0.0, 60.0};
  // Moon in the zenith of an equator observer: same direction, 1 Earth radius closer
  Equatorial t = topocentric(geo, 100.0, eq);
  CHECK_NEAR(t.ra_deg, 100.0, 1e-9);
  CHECK_NEAR(t.dec_deg, 0.0, 1e-9);
  CHECK_NEAR(t.dist, 59.0, 1e-9);
  // Observer 90 deg from the Moon direction: RA shifts by atan(1 / 60)
  t = topocentric(geo, 10.0, eq);
  CHECK_NEAR(wrap180(t.ra_deg - geo.ra_deg), atan(1.0 / 60.0) * 180.0 / 3.14159265358979, 1e-6);
}

TEST(astro_horizontal) {
  Horizontal h = toHorizontal(0.0, 37.5, 37.5);
  CHECK_NEAR(h.alt_deg, 90.0, 1e-6);
  h = toHorizontal(0.0, 0.0, 37.5);        // meridian, south
  CHECK_NEAR(h.alt_deg, 52.5, 1e-9);
  CHECK_NEAR(h.az_deg, 180.0, 1e-9);
  h = toHorizontal(90.0, 0.0, 37.5);       // 6 h after transit: setting in the west
  CHECK_NEAR(h.alt_deg, 0.0, 1e-9);
  CHECK_NEAR(h.az_deg, 270.0, 1e-9);
  h = toHorizontal(-90.0, 0.0, 37.5);
  CHECK_NEAR(h.az_deg, 90.0, 1e-9);
}

TEST(astro_moon_tracking_rate) {
  const Observer seoul{37.5665, 126.9780};
  double sum = 0, lo = 1e9, hi = -1e9;
  int n = 0;
  const double jd0 = julianDate({2026, 11, 1, 0, 0, 0.0});
  for (double k = 0; k < 29.5; k += 0.25) {       // one month, every 6 h
    const MoonState s = moonState(jd0 + k, seoul);
    sum += s.ha_rate_arcsec_s; n++;
    if (s.ha_rate_arcsec_s < lo) lo = s.ha_rate_arcsec_s;
    if (s.ha_rate_arcsec_s > hi) hi = s.ha_rate_arcsec_s;
  }
  CHECK_NEAR(sum / n, 14.49, 0.05);   // mean lunar rate ~3.7 % below sidereal 15.041
  CHECK(lo > 13.7);
  CHECK(hi < 15.0);
  CHECK_NEAR(SIDEREAL_RATE_ARCSEC_S, 15.0411, 0.0001);
}

TEST(astro_moon_rate_matches_hour_angle_change) {
  const Observer seoul{37.5665, 126.9780};
  const double jd = julianDate({2026, 11, 22, 13, 0, 0.0});
  const MoonState a = moonState(jd, seoul);
  const MoonState b = moonState(jd + 60.0 / 86400.0, seoul);
  const double measured = wrap180(b.ha_deg - a.ha_deg) * 3600.0 / 60.0;
  CHECK_NEAR(a.ha_rate_arcsec_s, measured, 0.01);
  CHECK_NEAR(a.dec_rate_arcsec_s, (b.topo.dec_deg - a.topo.dec_deg) * 3600.0 / 60.0, 0.01);
  CHECK_NEAR(a.hor.alt_deg, toHorizontal(a.ha_deg, a.topo.dec_deg, seoul.lat_deg).alt_deg, 1e-12);
}

// ---- MoonTracker / Telemetry glue
#include "../app/src/astro/moon_telemetry.h"

TEST(astro_utc_from_gps) {
  UtcTime u;
  CHECK(!utcFromGps(0, 0, 0, 0, 0, 0, 0, u));          // GPS before the first RMC
  CHECK(utcFromGps(26, 11, 22, 13, 5, 7, 250, u));
  CHECK(u.year == 2026 && u.month == 11 && u.day == 22 && u.hour == 13 && u.minute == 5);
  CHECK_NEAR(u.second, 7.25, 1e-12);
}

TEST(astro_moon_tracker_period_and_fix) {
  MoonTracker tr(1000);
  UtcTime u{2026, 11, 22, 12, 0, 0.0};
  const Observer seoul{37.5665, 126.9780};
  CHECK(!tr.update(0, false, &u, seoul));               // no fix
  CHECK(!tr.valid());
  CHECK(tr.update(100, true, &u, seoul));               // first fix: compute now
  CHECK(tr.valid());
  CHECK(!tr.update(1000, true, &u, seoul));             // 0.9 s later: keep
  CHECK(tr.update(1100, true, &u, seoul));              // 1 s later: compute
  CHECK(!tr.update(1200, true, nullptr, seoul));        // bad date
  CHECK(!tr.valid());
}

TEST(astro_moon_telemetry_matches_moon_state) {
  MoonTracker tr(1000);
  Telemetry t;
  t.now_ms = 5000;
  t.year = 26; t.month = 11; t.day = 22; t.hour = 12;
  t.fixquality = 1;
  t.latitude = 37.5665; t.longitude = 126.9780; t.altitude_m = 40.0f;
  CHECK(updateMoonTelemetry(tr, t));
  const MoonState s = moonState(julianDate({2026, 11, 22, 12, 0, 0.0}), Observer{37.5665, 126.9780, 40.0});
  CHECK(t.moon_valid);
  CHECK_NEAR(t.moon_ra_deg, s.topo.ra_deg, 1e-12);
  CHECK_NEAR(t.moon_dec_deg, s.topo.dec_deg, 1e-12);
  CHECK_NEAR(t.moon_alt_deg, s.hor.alt_deg, 1e-12);
  CHECK_NEAR(t.moon_rate_arcsec_s, s.ha_rate_arcsec_s, 1e-12);
  t.fixquality = 0;
  updateMoonTelemetry(tr, t);
  CHECK(!t.moon_valid);
}
