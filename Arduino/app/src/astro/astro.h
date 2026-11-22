// Sun, Moon and planet positions for telescope pointing and tracking.
//
// All internal math is double (64-bit). On the ESP8266 this is soft-float,
// but one full Moon update is a few ms, and it runs about once per second.
//
// Methods (see docs/MOON_POSITION.md):
//   Moon   : Schlyter low-precision elements + 12/5/2 perturbation terms (~2' error)
//   Sun    : Schlyter elements (~1')
//   Planets: Standish (JPL) J2000 Keplerian elements, valid 1800-2050, then
//            light-time and precession to the equinox of date (~1' Venus/Mars,
//            several ' for Jupiter/Saturn from the element accuracy)
//   GMST   : IAU 1982 formula
//   Topocentric Moon: vector subtraction of the observer position (WGS84)
//
// Angles are degrees unless the name says otherwise. Longitude east positive,
// latitude north positive (as GPS gives them).
#pragma once

namespace astro {

// Ephemeris time minus UT (TT - UT1). About 69 s in 2020-2030.
constexpr double DELTA_T_S = 69.0;

// Sidereal rate in arcsec per SI second (360.98564736629 deg / 86400 s).
constexpr double SIDEREAL_RATE_ARCSEC_S = 360.98564736629 * 3600.0 / 86400.0;

struct UtcTime {
  int year;      // full year, e.g. 2026
  int month;     // 1..12
  int day;       // 1..31
  int hour;
  int minute;
  double second; // may include milliseconds
};

struct Equatorial {
  double ra_deg;   // 0..360
  double dec_deg;  // -90..90
  double dist;     // Moon: Earth radii, Sun/planets: AU
};

struct Ecliptic {
  double lon_deg;
  double lat_deg;
  double dist;
};

struct Horizontal {
  double alt_deg;  // above the horizon
  double az_deg;   // from north, east positive, 0..360
};

struct Observer {
  double lat_deg;
  double lon_deg;     // east positive
  double height_m = 0;
};

enum class Planet { Mercury, Venus, Earth, Mars, Jupiter, Saturn };

// Things the telescope can point at and track. The order is used by the UI.
enum class Target { Moon, Mercury, Venus, Mars, Jupiter, Saturn };
constexpr int TARGET_COUNT = 6;
const char* targetName(Target t);

// ---- time
double julianDate(const UtcTime& t);           // Gregorian calendar -> JD (UT)
inline double ttFromUt(double jd_ut) { return jd_ut + DELTA_T_S / 86400.0; }
double gmstDeg(double jd_ut);                  // Greenwich mean sidereal time, 0..360
double lstDeg(double jd_ut, double lon_deg);   // local mean sidereal time, 0..360

// ---- angles
double wrap360(double deg);
double wrap180(double deg);                    // -180..180

// ---- bodies (jd_tt = ephemeris time)
double obliquityDeg(double jd_tt);             // mean obliquity of date
Ecliptic sunEcliptic(double jd_tt);            // geocentric, of date
Equatorial sunEquatorial(double jd_tt);
Ecliptic moonEcliptic(double jd_tt);           // geocentric, of date, dist in Earth radii
Equatorial moonGeocentric(double jd_tt);

// Heliocentric ecliptic J2000 position in AU (x, y, z).
void planetHeliocentric(Planet p, double jd_tt, double xyz[3]);
// Geocentric equatorial J2000 RA/Dec, distance in AU (geometric, no light-time).
Equatorial planetGeocentric(Planet p, double jd_tt);
// Geocentric RA/Dec of date, corrected for light-time, distance in AU.
Equatorial planetApparent(Planet p, double jd_tt);
// Precession J2000 -> mean equinox of date (Meeus 21.4).
Equatorial precessFromJ2000(const Equatorial& q, double jd_tt);

// ---- observer
// Geocentric -> topocentric (parallax). geo.dist must be in Earth radii.
Equatorial topocentric(const Equatorial& geo, double lst_deg, const Observer& obs);
Horizontal toHorizontal(double ha_deg, double dec_deg, double lat_deg);

// Everything a pointing/tracking screen and the RA motor need, from one UT instant.
struct TargetState {
  Equatorial geo;        // geocentric, of date (Moon: Earth radii, planets: AU)
  Equatorial topo;       // as seen from the observer (same distance unit)
  double lst_deg;
  double ha_deg;         // topocentric hour angle, -180..180
  Horizontal hor;
  double ha_rate_arcsec_s;   // RA-axis tracking rate (sidereal ~15.04, Moon ~14.5)
  double dec_rate_arcsec_s;
};
TargetState targetState(Target target, double jd_ut, const Observer& obs);

using MoonState = TargetState;
inline MoonState moonState(double jd_ut, const Observer& obs) {
  return targetState(Target::Moon, jd_ut, obs);
}

}  // namespace astro
