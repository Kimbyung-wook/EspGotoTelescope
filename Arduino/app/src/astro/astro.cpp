#include "astro.h"
#include <math.h>

namespace astro {
namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double D2R = PI / 180.0;
constexpr double R2D = 180.0 / PI;
constexpr double JD_J2000 = 2451545.0;
// Schlyter's day number d = JD - 2451543.5 (0.0 at 1999-12-31 0h).
constexpr double JD_SCHLYTER0 = 2451543.5;

inline double sind(double d) { return sin(d * D2R); }
inline double cosd(double d) { return cos(d * D2R); }

// Kepler's equation M = E - e sin E, Newton iteration, radians.
double solveKepler(double M_rad, double e) {
  double E = M_rad + e * sin(M_rad) * (1.0 + e * cos(M_rad));
  for (int i = 0; i < 10; i++) {
    const double dE = (E - e * sin(E) - M_rad) / (1.0 - e * cos(E));
    E -= dE;
    if (fabs(dE) < 1e-12) break;
  }
  return E;
}

Equatorial eclipticToEquatorial(double lon_deg, double lat_deg, double dist, double ecl_deg) {
  const double cl = cosd(lat_deg);
  const double x = cl * cosd(lon_deg);
  const double y0 = cl * sind(lon_deg);
  const double z0 = sind(lat_deg);
  const double ce = cosd(ecl_deg), se = sind(ecl_deg);
  const double y = y0 * ce - z0 * se;
  const double z = y0 * se + z0 * ce;
  Equatorial q;
  q.ra_deg = wrap360(atan2(y, x) * R2D);
  q.dec_deg = atan2(z, sqrt(x * x + y * y)) * R2D;
  q.dist = dist;
  return q;
}

// Standish (JPL) J2000 elements: a [AU], e, I, Omega, varpi, L [deg]
// and rates per Julian century (a, e per century; angles arcsec per century).
struct Elements { double x0[6]; double dx[6]; };
const Elements kPlanets[] = {
  {{0.38709893, 0.20563069, 7.00487, 48.33167, 77.45645, 252.25084},
   {0.00000066, 0.00002527, -23.51, -446.30, 573.57, 538101628.29}},      // Mercury
  {{0.72333199, 0.00677323, 3.39471, 76.68069, 131.53298, 181.97973},
   {0.00000092, -0.00004938, -2.86, -996.89, -108.80, 210664136.06}},     // Venus
  {{1.00000011, 0.01671022, 0.00005, -11.26064, 102.94719, 100.46435},
   {-0.00000005, -0.00003804, -46.94, -18228.25, 1198.28, 129597740.63}}, // Earth-Moon barycentre
  {{1.52366231, 0.09341233, 1.85061, 49.57854, 336.04084, 355.45332},
   {-0.00007221, 0.00011902, -25.47, -1020.19, 1560.78, 68905103.78}},    // Mars
  {{5.20336301, 0.04839266, 1.30530, 100.55615, 14.75385, 34.40438},
   {0.00060737, -0.00012880, -4.15, 1217.17, 839.93, 10925078.35}},       // Jupiter
  {{9.53707032, 0.05415060, 2.48446, 113.71504, 92.43194, 49.94432},
   {-0.00301530, -0.00036762, 6.11, -1591.05, -1948.89, 4401052.95}},     // Saturn
};
constexpr double OBLIQUITY_J2000 = 23.43928;

}  // namespace

double wrap360(double deg) {
  double r = fmod(deg, 360.0);
  return r < 0 ? r + 360.0 : r;
}

double wrap180(double deg) {
  double r = wrap360(deg);
  return r > 180.0 ? r - 360.0 : r;
}

double julianDate(const UtcTime& t) {
  int y = t.year, m = t.month;
  if (m <= 2) { y -= 1; m += 12; }
  const int A = y / 100;
  const int B = 2 - A + A / 4;
  const double day = t.day + (t.hour + (t.minute + t.second / 60.0) / 60.0) / 24.0;
  return floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1)) + day + B - 1524.5;
}

double gmstDeg(double jd_ut) {
  const double D = jd_ut - JD_J2000;
  const double T = D / 36525.0;
  return wrap360(280.46061837 + 360.98564736629 * D + T * T * (0.000387933 - T / 38710000.0));
}

double lstDeg(double jd_ut, double lon_deg) { return wrap360(gmstDeg(jd_ut) + lon_deg); }

double obliquityDeg(double jd_tt) { return 23.4393 - 3.563e-7 * (jd_tt - JD_SCHLYTER0); }

Ecliptic sunEcliptic(double jd_tt) {
  const double d = jd_tt - JD_SCHLYTER0;
  const double w = 282.9404 + 4.70935e-5 * d;
  const double e = 0.016709 - 1.151e-9 * d;
  const double M = wrap360(356.0470 + 0.9856002585 * d);
  const double E = solveKepler(M * D2R, e);
  const double xv = cos(E) - e;
  const double yv = sqrt(1.0 - e * e) * sin(E);
  Ecliptic s;
  s.lon_deg = wrap360(atan2(yv, xv) * R2D + w);
  s.lat_deg = 0.0;
  s.dist = sqrt(xv * xv + yv * yv);
  return s;
}

Equatorial sunEquatorial(double jd_tt) {
  const Ecliptic s = sunEcliptic(jd_tt);
  return eclipticToEquatorial(s.lon_deg, s.lat_deg, s.dist, obliquityDeg(jd_tt));
}

Ecliptic moonEcliptic(double jd_tt) {
  const double d = jd_tt - JD_SCHLYTER0;
  // Moon elements (of date)
  const double N = wrap360(125.1228 - 0.0529538083 * d);
  const double i = 5.1454;
  const double w = wrap360(318.0634 + 0.1643573223 * d);
  const double a = 60.2666;  // Earth radii
  const double e = 0.054900;
  const double M = wrap360(115.3654 + 13.0649929509 * d);

  const double E = solveKepler(M * D2R, e);
  const double xv = a * (cos(E) - e);
  const double yv = a * sqrt(1.0 - e * e) * sin(E);
  const double v = atan2(yv, xv) * R2D;
  double r = sqrt(xv * xv + yv * yv);

  const double vw = v + w;
  const double xh = r * (cosd(N) * cosd(vw) - sind(N) * sind(vw) * cosd(i));
  const double yh = r * (sind(N) * cosd(vw) + cosd(N) * sind(vw) * cosd(i));
  const double zh = r * (sind(vw) * sind(i));
  double lon = atan2(yh, xh) * R2D;
  double lat = atan2(zh, sqrt(xh * xh + yh * yh)) * R2D;

  // Perturbations
  const double Ms = wrap360(356.0470 + 0.9856002585 * d);
  const double ws = 282.9404 + 4.70935e-5 * d;
  const double Ls = Ms + ws;
  const double Lm = M + w + N;
  const double D = Lm - Ls;   // mean elongation
  const double F = Lm - N;    // argument of latitude
  const double Mm = M;

  lon += -1.274 * sind(Mm - 2 * D)   // evection
         + 0.658 * sind(2 * D)       // variation
         - 0.186 * sind(Ms)          // yearly equation
         - 0.059 * sind(2 * Mm - 2 * D)
         - 0.057 * sind(Mm - 2 * D + Ms)
         + 0.053 * sind(Mm + 2 * D)
         + 0.046 * sind(2 * D - Ms)
         + 0.041 * sind(Mm - Ms)
         - 0.035 * sind(D)           // parallactic equation
         - 0.031 * sind(Mm + Ms)
         - 0.015 * sind(2 * F - 2 * D)
         + 0.011 * sind(Mm - 4 * D);
  lat += -0.173 * sind(F - 2 * D)
         - 0.055 * sind(Mm - F - 2 * D)
         - 0.046 * sind(Mm + F - 2 * D)
         + 0.033 * sind(F + 2 * D)
         + 0.017 * sind(2 * Mm + F);
  r += -0.58 * cosd(Mm - 2 * D) - 0.46 * cosd(2 * D);

  Ecliptic m;
  m.lon_deg = wrap360(lon);
  m.lat_deg = lat;
  m.dist = r;
  return m;
}

Equatorial moonGeocentric(double jd_tt) {
  const Ecliptic m = moonEcliptic(jd_tt);
  return eclipticToEquatorial(m.lon_deg, m.lat_deg, m.dist, obliquityDeg(jd_tt));
}

void planetHeliocentric(Planet p, double jd_tt, double xyz[3]) {
  const Elements& el = kPlanets[static_cast<int>(p)];
  const double T = (jd_tt - JD_J2000) / 36525.0;
  const double a = el.x0[0] + el.dx[0] * T;
  const double e = el.x0[1] + el.dx[1] * T;
  const double I = el.x0[2] + el.dx[2] / 3600.0 * T;
  const double Om = el.x0[3] + el.dx[3] / 3600.0 * T;
  const double vp = el.x0[4] + el.dx[4] / 3600.0 * T;
  const double L = el.x0[5] + el.dx[5] / 3600.0 * T;
  const double w = vp - Om;              // argument of perihelion
  const double M = wrap180(L - vp);      // mean anomaly
  const double E = solveKepler(M * D2R, e);
  const double xp = a * (cos(E) - e);
  const double yp = a * sqrt(1.0 - e * e) * sin(E);
  const double cw = cosd(w), sw = sind(w), cO = cosd(Om), sO = sind(Om), cI = cosd(I), sI = sind(I);
  xyz[0] = (cw * cO - sw * sO * cI) * xp + (-sw * cO - cw * sO * cI) * yp;
  xyz[1] = (cw * sO + sw * cO * cI) * xp + (-sw * sO + cw * cO * cI) * yp;
  xyz[2] = (sw * sI) * xp + (cw * sI) * yp;
}

Equatorial planetGeocentric(Planet p, double jd_tt) {
  double pl[3], ea[3];
  planetHeliocentric(p, jd_tt, pl);
  planetHeliocentric(Planet::Earth, jd_tt, ea);
  const double x = pl[0] - ea[0], y0 = pl[1] - ea[1], z0 = pl[2] - ea[2];
  const double dist = sqrt(x * x + y0 * y0 + z0 * z0);
  const double lon = atan2(y0, x) * R2D;
  const double lat = atan2(z0, sqrt(x * x + y0 * y0)) * R2D;
  return eclipticToEquatorial(lon, lat, dist, OBLIQUITY_J2000);
}

Equatorial topocentric(const Equatorial& geo, double lst_deg, const Observer& obs) {
  // Observer position in Earth equatorial radii (WGS84, Meeus ch. 11).
  constexpr double F = 1.0 / 298.257223563;
  constexpr double R_EQ_M = 6378137.0;
  const double u = atan((1.0 - F) * tan(obs.lat_deg * D2R));
  const double h = obs.height_m / R_EQ_M;
  const double rho_sin = (1.0 - F) * sin(u) + h * sind(obs.lat_deg);
  const double rho_cos = cos(u) + h * cosd(obs.lat_deg);

  const double cd = cosd(geo.dec_deg);
  const double x = geo.dist * cd * cosd(geo.ra_deg) - rho_cos * cosd(lst_deg);
  const double y = geo.dist * cd * sind(geo.ra_deg) - rho_cos * sind(lst_deg);
  const double z = geo.dist * sind(geo.dec_deg) - rho_sin;
  Equatorial t;
  t.dist = sqrt(x * x + y * y + z * z);
  t.ra_deg = wrap360(atan2(y, x) * R2D);
  t.dec_deg = asin(z / t.dist) * R2D;
  return t;
}

Horizontal toHorizontal(double ha_deg, double dec_deg, double lat_deg) {
  const double sl = sind(lat_deg), cl = cosd(lat_deg);
  const double sdc = sind(dec_deg), cdc = cosd(dec_deg), ch = cosd(ha_deg);
  Horizontal h;
  h.alt_deg = asin(sl * sdc + cl * cdc * ch) * R2D;
  h.az_deg = wrap360(atan2(-cdc * sind(ha_deg), sdc * cl - cdc * sl * ch) * R2D);
  return h;
}

namespace {
Equatorial moonTopoAt(double jd_ut, const Observer& obs, double* lst_out) {
  const double lst = lstDeg(jd_ut, obs.lon_deg);
  if (lst_out) *lst_out = lst;
  return topocentric(moonGeocentric(ttFromUt(jd_ut)), lst, obs);
}
}  // namespace

MoonState moonState(double jd_ut, const Observer& obs) {
  MoonState s;
  s.geo = moonGeocentric(ttFromUt(jd_ut));
  s.lst_deg = lstDeg(jd_ut, obs.lon_deg);
  s.topo = topocentric(s.geo, s.lst_deg, obs);
  s.ha_deg = wrap180(s.lst_deg - s.topo.ra_deg);
  s.hor = toHorizontal(s.ha_deg, s.topo.dec_deg, obs.lat_deg);

  // Rates from a +-30 s central difference (double keeps ~1e-5 "/s here).
  constexpr double DT_S = 30.0;
  const double dt_day = DT_S / 86400.0;
  double lst_a, lst_b;
  const Equatorial a = moonTopoAt(jd_ut - dt_day, obs, &lst_a);
  const Equatorial b = moonTopoAt(jd_ut + dt_day, obs, &lst_b);
  const double dha = wrap180((lst_b - b.ra_deg) - (lst_a - a.ra_deg));
  s.ha_rate_arcsec_s = dha * 3600.0 / (2.0 * DT_S);
  s.dec_rate_arcsec_s = (b.dec_deg - a.dec_deg) * 3600.0 / (2.0 * DT_S);
  return s;
}

}  // namespace astro
