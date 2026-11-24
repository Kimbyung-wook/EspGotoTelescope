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
// JD (UT) -> calendar date and time (Meeus 7.3); false for jd < 0
bool utcFromJd(double jd_ut, UtcTime& out);
int daysInMonth(int year, int month);       // Gregorian, 28..31 (31 for a bad month)

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

// ---- telescope pointing from gravity, for a polar-aligned equatorial mount
// Tube frame: x = optical axis, y = Dec axis, z = x cross y (toward increasing Dec
// on the normal side). axis_ha_deg is the RA-axis angle (0: Dec axis horizontal,
// east-west, tube on the meridian), dec_axis_deg the Dec-axis angle; |dec_axis| > 90
// is the flipped (other pier side) pose.
// Forward model: the local "up" unit vector seen in the tube frame.
void upInTubeFrame(double axis_ha_deg, double dec_axis_deg, double lat_deg, double up[3]);

struct MountPose { double ha_deg; double dec_deg; };   // where the tube points (HA -180..180)
// Gravity alone leaves two answers, mirror images about the meridian. Returns
// both (pointing HA/Dec, flipped poses already folded back). False if 'up' is
// inconsistent with the latitude (mount not polar aligned or bad data).
bool poseCandidatesFromUp(const double up[3], double lat_deg, MountPose out[2]);
// Picks the candidate closest to a hint direction (normally the chosen target).
bool pointingFromUp(const double up[3], double lat_deg, double lst_deg,
                    double hint_ra_deg, double hint_dec_deg, Equatorial& out);

// ---- pointing guidance (DES-PNT-04)
// Target minus tube: dRA (deg, wrapped -180..180, positive = target further east,
// turn the tube east) and dDec (deg, positive = turn north).
void guideOffsets(double scope_ra_deg, double scope_dec_deg, double target_ra_deg,
                  double target_dec_deg, double& dra_deg, double& ddec_deg);
// Both offsets inside tol (dRA measured on the sky: dRA * cos(dec))
bool onTarget(double dra_deg, double ddec_deg, double dec_deg, double tol_deg);

// ---- polar alignment (DES-AST-02, DES-PA-03)
// Polaris (alpha UMi), J2000, precessed to the date
Equatorial polarisOfDate(double jd_ut);
// Hour angle of Polaris, -180..180
double polarisHaDeg(double jd_ut, double lon_deg);
// Where Polaris sits around the pole as a clock hand (12 = above the pole, seen
// with the naked eye facing north; an inverting polar scope shows it 6 h away).
// Returns 0 < hours <= 12.
double polarisClockHours(double ha_deg);

}  // namespace astro
