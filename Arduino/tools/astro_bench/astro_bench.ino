// On-device measurement test for src/astro (NodeMCU v3 / ESP8266).
//
// What it checks
//   1. Accuracy on the target: the Meeus worked examples must give the same
//      numbers as the host unit tests (test/test_astro.cpp). This proves
//      double really is 64-bit on the ESP8266 toolchain.
//   2. Cost: microseconds per call for each astro function, and double vs
//      float sin() for reference.
// Results go to Serial (115200) and the OLED.
//
// Build:  SKETCH=tools/astro_bench scripts/build.sh --library app/src/astro
// Upload: add  --upload -p COM3  (disconnect the GPS TX wire first: GPS shares UART0)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <astro.h>
#include "bench_timer.h"

using namespace astro;

static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool oled_ok = false;
static char lines[8][22];
static int nline = 0;
volatile double sink = 0;  // keeps the compiler from removing benchmark loops

static void out(const char* fmt, ...) {
  char buf[96];
  va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
  Serial.println(buf);
  if (nline < 8) { strncpy(lines[nline], buf, 21); lines[nline][21] = 0; nline++; }
}

static void showOled() {
  if (!oled_ok) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < nline; i++) { oled.setCursor(0, i * 8); oled.print(lines[i]); }
  oled.display();
  nline = 0;
}

static bool near(double a, double b, double tol) { return fabs(a - b) <= tol; }

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin();
  oled_ok = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println();
  Serial.println("=== astro_bench ===");

  // ---- 1. accuracy on target
  const Equatorial m = moonGeocentric(2448724.5);           // Meeus 47.a
  const double gm = gmstDeg(2446896.30625);                  // Meeus 12.b
  const double mars = planetGeocentric(Planet::Mars, ttFromUt(julianDate({2003, 8, 27, 12, 0, 0.0}))).dist;
  const bool ok = sizeof(double) == 8 &&
                  near(m.ra_deg, 134.688470, 3.0 / 60) && near(m.dec_deg, 13.768368, 3.0 / 60) &&
                  near(gm, 128.7378734, 1e-5) && near(mars, 0.3727, 0.002);
  out("CPU %u MHz dbl=%uB", ESP.getCpuFreqMHz(), (unsigned)sizeof(double));
  out("Moon RA %10.6f", m.ra_deg);
  out("Moon DE %10.6f", m.dec_deg);
  out("GMST    %11.7f", gm);
  out("Mars    %8.5f AU", mars);
  out("Accuracy: %s", ok ? "PASS" : "FAIL");
  Serial.println("(host reference: RA 134.665891 DE 13.764429 GMST 128.7378732 Mars 0.37293)");
  showOled();
  delay(3000);

  // ---- 2. cost per call
  const double jd0 = julianDate({2026, 11, 22, 12, 0, 0.0});
  const Observer obs{37.5665, 126.9780, 40.0};
  const float t_sin = timeUs(2000, [](int i) { sink += sin(0.001 * i); });
  const float t_sinf = timeUs(2000, [](int i) { sink += sinf(0.001f * i); });
  const float t_jd = timeUs(500, [](int i) { sink += julianDate({2026, 11, 22, 12, i % 60, 0.0}); });
  const float t_gmst = timeUs(500, [&](int i) { sink += gmstDeg(jd0 + i * 1e-5); });
  const float t_sun = timeUs(200, [&](int i) { sink += sunEquatorial(jd0 + i * 1e-3).ra_deg; });
  const float t_moon = timeUs(200, [&](int i) { sink += moonGeocentric(jd0 + i * 1e-3).ra_deg; });
  const float t_state = timeUs(50, [&](int i) { sink += targetState(Target::Moon, jd0 + i * 1e-3, obs).ha_rate_arcsec_s; });
  const float t_mars = timeUs(200, [&](int i) { sink += planetGeocentric(Planet::Mars, jd0 + i).ra_deg; });

  out("us/call @%uMHz", ESP.getCpuFreqMHz());
  out("sin %5.1f sinf %5.1f", t_sin, t_sinf);
  out("JD %6.1f GMST %6.1f", t_jd, t_gmst);
  out("Sun      %8.1f", t_sun);
  out("Moon geo %8.1f", t_moon);
  out("MoonState%8.1f", t_state);
  out("Mars     %8.1f", t_mars);
  out("1Hz load %6.2f %%", t_state / 1e4);   // MoonState once per second
  showOled();

  const TargetState s = targetState(Target::Moon, jd0, obs);
  Serial.println("(host reference: RA 27.5549 DE 15.7577 HA -19.0878 alt 62.435 az 137.147 rate 14.6677)");
  Serial.printf("Seoul 2026-11-22 12:00 UT: RA %.4f DE %.4f HA %.4f alt %.3f az %.3f rate %.4f\"/s\n",
                s.topo.ra_deg, s.topo.dec_deg, s.ha_deg, s.hor.alt_deg, s.hor.az_deg, s.ha_rate_arcsec_s);
}

void loop() { delay(1000); }
