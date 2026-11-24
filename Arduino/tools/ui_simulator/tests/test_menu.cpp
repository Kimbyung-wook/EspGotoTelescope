// Simulator tests: the whole loop with the real firmware UI (SimpleGoToUi):
// protocol message -> sensors -> app::Core -> screen lines and motor output.
// Drives sim::Session, the same code path the GUI uses, so the protocol layer is
// covered too. Here: boot, reset, protocol, sensor values on screen, estimator,
// motor output, flash across a reset. How one screen reacts to the dial and the
// button is tested by the unit tests (test/test_ui.cpp), not repeated here.
// Build and run with ../run_tests.sh.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "firmware_ui.h"
#include "protocol.h"
#include "ui/simplegoto_ui.h"
#include "config/settings.h"
#include "version.h"

// Boot screen line 0 (from version.h, so a version bump needs no test change)
#define BOOT_TITLE SIMPLEGOTO_NAME " v" SIMPLEGOTO_VERSION

namespace {

int g_checks = 0;
int g_failures = 0;
const char* g_test = "";

#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failures++; \
  std::printf("  FAIL [%s] %s:%d  %s\n", g_test, __FILE__, __LINE__, #cond); } } while (0)

std::string num(int v) { char b[16]; std::snprintf(b, sizeof(b), "%d", v); return b; }

// Test harness around a Session with the firmware backend.
struct Sim {
  sim::FirmwareUi ui;
  sim::Session session{&ui};
  std::vector<std::string> replies;

  void send(const std::string& msg) {
    replies.clear();
    session.handleLine(msg, replies);
  }
  void tick(int n = 1) { send("{\"type\":\"tick\",\"n\":" + num(n) + "}"); }
  void rotate(int n) { send("{\"type\":\"input\",\"rotate\":" + num(n) + "}"); tick(); }
  void click() { send("{\"type\":\"input\",\"click\":1}"); tick(); }
  void boot() { tick(sim::FirmwareUi::kBootFrames + 1); }
  const char* line(int i) const { return session.lastFrame().lines[i]; }
  const sim::MotorOutput& motor() const { return session.lastMotor(); }
  int depth() { return ui.ui().manager().depth(); }
};

// '?' in the pattern matches the blinking cursor ('>' or '-') or a literal '?'.
bool lineMatches(const char* got, const char* pattern) {
  size_t n = std::strlen(pattern);
  if (std::strlen(got) != n) return false;
  for (size_t i = 0; i < n; i++) {
    if (pattern[i] == '?') { if (got[i] != '>' && got[i] != '-' && got[i] != '?') return false; }
    else if (pattern[i] != got[i]) return false;
  }
  return true;
}

void expectLine(Sim& s, int i, const char* pattern, int src_line) {
  g_checks++;
  if (!lineMatches(s.line(i), pattern)) {
    g_failures++;
    std::printf("  FAIL [%s] (test line %d) screen line %d: got \"%s\" expected \"%s\"\n",
                g_test, src_line, i, s.line(i), pattern);
  }
}
#define EXPECT_LINE(s, i, pat) expectLine(s, i, pat, __LINE__)

// Expect the title and content area (lines 1..7) exactly.
void expectScreen(Sim& s, const char* const (&pat)[7], int src_line) {
  for (int i = 0; i < 7; i++) expectLine(s, i + 1, pat[i], src_line);
}
#define EXPECT_SCREEN(s, ...) do { const char* const _p[7] = __VA_ARGS__; expectScreen(s, _p, __LINE__); } while (0)

// Main menu item k -> its screen (cursor starts on the first item)
void openMain(Sim& s, int k) { s.rotate(-20); s.rotate(k); s.click(); }
// RA Speed: set RRcmd to v and go back to the main menu
void setRrcmd(Sim& s, int v) {
  openMain(s, 5);
  s.click(); s.rotate(v); s.click();
  s.rotate(2); s.click();
}

/******************************************************************/

void test_boot_then_main_menu() {
  Sim s;
  s.tick();
  EXPECT_LINE(s, 0, BOOT_TITLE);
  EXPECT_LINE(s, 1, "");
  s.tick(sim::FirmwareUi::kBootLineFrames * 7);   // 8 lines: the screen is full
  EXPECT_LINE(s, 1, "1. Display ON 160MHz");
  EXPECT_LINE(s, 2, "2. IMU is ON");
  EXPECT_LINE(s, 6, "3. GPS is ON");
  EXPECT_LINE(s, 7, "4. Buttons are ON");
  s.tick(sim::FirmwareUi::kBootLineFrames * 2);   // 10 lines: scrolled, line 0 stays
  EXPECT_SCREEN(s, {"Acc +-16G", "Gyro 250 dps", "Mag 10 Hz", "3. GPS is ON",
                    "4. Buttons are ON", "5. GPIO/PWM is ON", "6. Settings new"});
  EXPECT_LINE(s, 0, BOOT_TITLE);
  s.tick(sim::FirmwareUi::kBootHoldFrames - 1);
  EXPECT_LINE(s, 7, "6. Settings new");   // boot log is still held
  s.tick();
  CHECK(!s.ui.booting());
  EXPECT_LINE(s, 0, "T 12: 0: 0.0 V 12.60");
  EXPECT_SCREEN(s, {"Main Menu", "?1Target", " 2Polar Align", " 3Status",
                    " 4Calibration", " 5Settings", " 6RA Speed"});
  CHECK(s.motor().in1 == 0 && s.motor().in2 == 0 && s.motor().pwm == 0);
}

void test_boot_failures() {
  Sim s;
  s.send("{\"type\":\"reset\",\"imu_ok\":false,\"gps_ok\":false}");
  s.tick(sim::FirmwareUi::kBootLineFrames * 8);
  EXPECT_LINE(s, 2, ">> IMU failed");
  EXPECT_LINE(s, 6, ">> GPS is timeout");
}

void test_input_ignored_during_boot() {
  Sim s;
  s.send("{\"type\":\"input\",\"rotate\":3,\"click\":1}");
  s.boot();
  EXPECT_LINE(s, 2, "?1Target");
  CHECK(s.depth() == 1);
}

void test_reset_restarts_ui() {
  Sim s;
  s.boot();
  s.rotate(4);
  s.click();
  CHECK(s.depth() == 2);
  s.send("{\"type\":\"reset\"}");
  s.boot();
  CHECK(s.depth() == 1);
  EXPECT_LINE(s, 2, "?1Target");
}

void test_ra_speed_edit_drives_motor() {
  Sim s;
  s.send("{\"type\":\"sensors\",\"imu\":{\"ax\":0.5}}");
  s.boot();
  s.rotate(5);
  s.click();
  EXPECT_SCREEN(s, {"*6RA Speed", "?RRcmd    0.00000PWM", " RRate    0.00000DPS", " Quit",
                    " ROL    0.00000  DEG", " LAT N 37.566500DEG", " PIT   -2.91874  DEG"});
  s.click();   // RRcmd -> edit mode
  s.rotate(5);
  EXPECT_LINE(s, 2, "?RRcmd    5.00000PWM");
  CHECK(s.motor().in1 == 0 && s.motor().in2 == 1 && s.motor().pwm == 5);
  s.rotate(-12);
  EXPECT_LINE(s, 2, "?RRcmd   -7.00000PWM");
  CHECK(s.motor().in1 == 1 && s.motor().in2 == 0 && s.motor().pwm == 7);
  s.rotate(7);
  CHECK(s.motor().in1 == 0 && s.motor().in2 == 0 && s.motor().pwm == 0);
  s.rotate(300);
  CHECK(s.motor().pwm == 255);   // duty limited by RaMotor
  s.click();   // finish editing
  EXPECT_LINE(s, 2, "?RRcmd  300.00000PWM");
  s.rotate(2);
  s.click();   // Quit
  EXPECT_LINE(s, 1, "Main Menu");
  CHECK(s.motor().pwm == 255);   // motor keeps running after leaving
}

void test_gps_screen_shows_sensor_values() {
  Sim s;
  s.boot();
  s.send("{\"type\":\"sensors\",\"gps\":{\"year\":26,\"month\":3,\"day\":9,\"hour\":21,"
         "\"minute\":5,\"seconds\":7,\"milliseconds\":450,\"lat\":-33.5,\"lon\":-70.25,"
         "\"fix\":2,\"sats\":11}}");
  openMain(s, 2);
  EXPECT_SCREEN(s, {"*3Status", "?GPS", " Acc/Gyro", " Mag/Temp", " IMU Calib",
                    " Att.Estimator", " Health"});
  s.click();
  EXPECT_LINE(s, 0, "T 21: 5: 7.4 V 12.60");
  EXPECT_SCREEN(s, {"*GPS", "Date: 2026/03/09", "Time:   21:05:07.450", " DGPS Sat 11",
                    "Pos GPS  Time GPS", " S -33.50000000 DEG", " W -70.25000000 DEG"});
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":0,\"sats\":0}}");
  s.tick();
  EXPECT_LINE(s, 4, "NoFix Sat  0");
  EXPECT_LINE(s, 5, "Pos ---  Time ---");   // no last fix yet (30 s needed)
  s.send("{\"type\":\"sensors\",\"gps\":{\"link\":false}}");   // GPS unplugged
  s.tick();
  EXPECT_LINE(s, 4, "NoFix Sat  0 NoLink");
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":1,\"sats\":8}}");   // last values were a fix
  s.tick();
  EXPECT_LINE(s, 4, "  GPS Sat  8 NoLink");
  EXPECT_LINE(s, 5, "Pos ---  Time ---");   // a stale fix is not used
  s.send("{\"type\":\"sensors\",\"gps\":{\"link\":1}}");
  s.tick();
  EXPECT_LINE(s, 4, "  GPS Sat  8");
  EXPECT_LINE(s, 5, "Pos GPS  Time GPS");
}

// DES-LOC-02: a fix held for 30 s is kept (saved once); without GPS it is used,
// also after a power cycle.
void test_last_gps_fix_is_kept() {
  Sim s;
  s.boot();
  openMain(s, 2);
  s.click();                 // GPS
  s.tick(290);
  CHECK(s.ui.settingsStore().writes == 0);
  s.tick(20);                // past 30 s of fix
  CHECK(s.ui.settingsStore().writes == 1);
  s.tick(400);               // once per power-on
  CHECK(s.ui.settingsStore().writes == 1);
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":0,\"sats\":0}}");
  s.tick();
  EXPECT_LINE(s, 5, "Pos LAST Time ---");
  s.send("{\"type\":\"reset\"}");
  s.boot();
  openMain(s, 2);
  s.click();
  EXPECT_LINE(s, 5, "Pos LAST Time ---");
}

// DES-LOC-02: a fix with a poor HDOP (above 2) is used while it lasts, but not kept
void test_last_gps_needs_good_hdop() {
  Sim s;
  s.boot();
  s.send("{\"type\":\"sensors\",\"gps\":{\"hdop\":3.5}}");
  openMain(s, 2);
  s.click();                 // GPS
  s.tick(400);               // 40 s
  CHECK(s.ui.settingsStore().writes == 0);
  EXPECT_LINE(s, 5, "Pos GPS  Time GPS");
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":0,\"sats\":0}}");
  s.tick();
  EXPECT_LINE(s, 5, "Pos ---  Time ---");
}

void test_imu_screens() {
  Sim s;
  s.boot();
  s.send("{\"type\":\"sensors\",\"imu\":{\"ax\":-4.903325,\"ay\":0,\"az\":8.492806,"
         "\"wx\":1.25,\"wy\":-0.5,\"wz\":15,\"mx\":0,\"my\":-30,\"mz\":-40,\"temp\":25.5}}");
  openMain(s, 2);
  s.rotate(1);
  s.click();
  // Firmware math is single-precision float: asinf(0.5) -> 30.00001 deg below.
  EXPECT_SCREEN(s, {"*Acc/Gyro", "Ax      -4.90333 mpss", "Ay       0.00000 mpss",
                    "Az       8.49281 mpss", "Wx       1.25000 dps", "Wy      -0.50000 dps",
                    "Wz      15.00000 dps"});
  s.click();
  s.rotate(1);
  s.click();
  EXPECT_SCREEN(s, {"*Mag/Temp", "Mx      0.00000 uT", "My    -30.00000 uT",
                    "Mz    -40.00000 uT", "Temp   25.50000 'c ", " ", " "});
  s.click();
  s.rotate(2);
  s.click();
  // estimator output is checked in test_att_estimator_*; here the raw rows
  EXPECT_LINE(s, 1, "*Att.Estimator");
  EXPECT_LINE(s, 4, "aR    0.00 aP   30.00");
  EXPECT_LINE(s, 5, "Wx        1.25000 DPS");
  EXPECT_LINE(s, 6, "Wy       -0.50000 DPS");
  EXPECT_LINE(s, 7, "Wz       15.00000 DPS");
}

// Moon from src/astro: default GPS 2026-11-21 12:00:00 UT, Seoul (host values of targetState)
void test_moon_target() {
  Sim s;
  s.boot();
  openMain(s, 0);
  s.rotate(-1);
  s.click();                 // Moon : lunar rate by default
  s.tick();
  EXPECT_LINE(s, 6, "ALT +61.64 AZ 165.89");
  EXPECT_LINE(s, 7, "?Track OFF 14.7143\"/s");
  s.rotate(-2);              // scroll up to the target lines
  EXPECT_LINE(s, 1, "*Moon");
  EXPECT_LINE(s, 2, "R.A.    14.22977 DEG");
  EXPECT_LINE(s, 3, "DEC.     9.86343 DEG");
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":0}}");
  s.tick(1);
  EXPECT_LINE(s, 1, "*Moon    no loc/time");
  EXPECT_LINE(s, 2, "R.A.         ---");
}

// DES-CAL-03 and DES-SYS-02: the motor stops while calibrating, then runs again.
void test_gyro_bias_calibration() {
  Sim s;
  s.boot();
  setRrcmd(s, 40);
  CHECK(s.motor().pwm == 40);
  s.send("{\"type\":\"sensors\",\"imu\":{\"wx\":0.5,\"wy\":-0.25,\"wz\":0.125}}");
  openMain(s, 3);
  s.click();
  EXPECT_LINE(s, 1, "*Gyro Calib");
  EXPECT_LINE(s, 2, "Keep still  0/30 s");
  CHECK(s.depth() == 3);
  CHECK(s.motor().pwm == 0);
  s.tick(320);
  EXPECT_LINE(s, 2, "Done. Apply?");
  CHECK(s.motor().pwm == 0);
  s.click();                 // Apply
  EXPECT_LINE(s, 2, "?Gyro Bias");   // back on the item we came from
  CHECK(s.motor().pwm == 40);
  s.rotate(2);
  s.click();                 // Back
  openMain(s, 2);
  s.rotate(3);
  s.click();                 // IMU Calib shows the new bias
  EXPECT_LINE(s, 1, "*IMU Calib");
  EXPECT_LINE(s, 6, "bWy    -0.25000 dps");
}

// Reads the number in "ROLL   %10.5f DEG" / "PITCH  %10.5f DEG".
double valueOf(const char* line) {
  double v = 0;
  const char* p = line;
  while (*p && *p != ' ') p++;
  std::sscanf(p, "%lf", &v);
  return v;
}

void openAttEst(Sim& s) { openMain(s, 2); s.rotate(4); s.click(); }

// The complementary filter (tau 1 s) starts at the first accelerometer tilt
// and then follows a tilt step with a 1 s time constant.
void test_att_estimator_converges() {
  Sim s;
  s.send("{\"type\":\"sensors\",\"imu\":{\"wx\":0,\"wy\":0,\"wz\":0}}");
  s.boot();
  openAttEst(s);
  EXPECT_LINE(s, 1, "*Att.Estimator");
  EXPECT_LINE(s, 3, "PITCH     0.00000 DEG");   // level
  s.send("{\"type\":\"sensors\",\"imu\":{\"ax\":-4.903325,\"az\":8.492806}}");
  s.tick(1);
  EXPECT_LINE(s, 4, "aR    0.00 aP   30.00");
  double first = valueOf(s.line(3));
  CHECK(std::fabs(first - 30.0 * (1.0 - std::exp(-0.1))) < 0.05);   // one step
  s.tick(10);    // 1 s: ~63 %
  double one_tau = valueOf(s.line(3));
  CHECK(one_tau > 17.0 && one_tau < 22.0);
  s.tick(100);   // 10 s
  CHECK(std::fabs(valueOf(s.line(3)) - 30.0) < 0.01);
  CHECK(std::fabs(valueOf(s.line(2))) < 0.01);
}

// After the gyro bias calibration, the estimator rows show bias-free rates.
void test_att_estimator_uses_gyro_bias() {
  Sim s;
  s.send("{\"type\":\"sensors\",\"imu\":{\"wx\":0.5,\"wy\":-0.25,\"wz\":0.125}}");
  s.boot();
  openAttEst(s);
  EXPECT_LINE(s, 5, "Wx        0.50000 DPS");   // no bias yet
  s.click();
  s.rotate(10); s.click();   // Back
  openMain(s, 3);
  s.click();                 // Gyro Bias
  s.tick(320);
  EXPECT_LINE(s, 2, "Done. Apply?");
  s.click();
  s.rotate(2); s.click();    // Back
  openAttEst(s);
  EXPECT_LINE(s, 1, "*Att.Estimator");
  EXPECT_LINE(s, 5, "Wx        0.00000 DPS");
  EXPECT_LINE(s, 6, "Wy        0.00000 DPS");
  EXPECT_LINE(s, 7, "Wz        0.00000 DPS");
}

// DES-PA: the tube altitude against the latitude; the motor is stopped meanwhile.
void test_polar_align() {
  Sim s;
  s.boot();
  setRrcmd(s, 30);
  openMain(s, 1);
  CHECK(s.motor().pwm == 0);
  EXPECT_LINE(s, 1, "*2Polar Align");
  EXPECT_LINE(s, 2, "ALT  +0.00 LAT +37.57");   // default IMU: level tube
  EXPECT_LINE(s, 3, "dALT -37.57 UP");
  EXPECT_LINE(s, 4, "dAZ    --- no mag");
  EXPECT_LINE(s, 6, "State: adjust");
  EXPECT_LINE(s, 7, "?(Confirm: wait OK)");
  s.rotate(-1);              // scroll up to the first line
  EXPECT_LINE(s, 2, "Dec 90: tube // axis");
  s.rotate(2);
  s.click();                 // Back
  EXPECT_LINE(s, 1, "Main Menu");
  CHECK(s.motor().pwm == 30);
}

// Target with the default GPS (2026-11-21 12:00:00 UT, Seoul); values from targetState().
// RA_DUTY_AT_SIDEREAL is 0 (not calibrated), so only the Offset moves the motor.
void test_target_tracking() {
  Sim s;
  s.boot();
  openMain(s, 0);
  EXPECT_LINE(s, 1, "*1Target");
  EXPECT_LINE(s, 3, "?Mercury");
  s.rotate(2);
  s.click();
  EXPECT_LINE(s, 1, "*Mars    computing...");   // the first frame after choosing
  s.tick(1);
  // T.R.A./T.DEC.: the default IMU is level, so the tube points at the horizon; of the two
  // mirror answers the one nearer Mars (north horizon, Dec = 90 - lat) is shown. '?': the
  // mount is not confirmed polar aligned.
  EXPECT_SCREEN(s, {"*Mars", "T.R.A. 187.48143 DEG?", "T.DEC.  52.43350 DEG?",
                    "dRA   -145.66 min W", "dDEC   -38.38 deg S", "ALT -28.09 AZ  40.75",
                    "?Track OFF 15.0411\"/s"});
  s.rotate(-2);
  EXPECT_LINE(s, 2, "R.A.   151.06717 DEG");
  EXPECT_LINE(s, 3, "DEC.    14.04876 DEG");
  s.rotate(2);
  s.click();   // Track on (sidereal rate, DES-TRK-02)
  EXPECT_LINE(s, 7, "?Track ON  15.0411\"/s");
  CHECK(s.motor().pwm == 0);   // not calibrated and no offset: the motor stays still
  s.rotate(2);
  s.click();   // edit Offset
  s.rotate(20);
  EXPECT_LINE(s, 7, "?Offset    +20.0 PWM");
  CHECK(s.motor().pwm == 20);  // the offset alone drives the motor
  s.click();   // done
  s.rotate(1);
  EXPECT_LINE(s, 7, "?Back   (RA uncal)");
  s.click();   // Back to the target list
  EXPECT_LINE(s, 5, "?Mars     TRACKING");
  CHECK(s.motor().pwm == 20);
}

// Settings survive a reset (power cycle): the simulator keeps its "flash" across begin().
void test_settings_survive_reset() {
  Sim s;
  s.boot();
  setRrcmd(s, 40);
  openMain(s, 4);
  EXPECT_SCREEN(s, {"*5Settings", "Flash: defaults", "RAduty      +0.0 PWM", "?RAduty = RRcmd",
                    " Location >", " Time >", " Motor dir Normal"});
  s.click();                 // RAduty = RRcmd -> saved by the main loop after this frame
  s.tick();
  EXPECT_LINE(s, 2, "Flash: saved");
  EXPECT_LINE(s, 3, "RAduty     +40.0 PWM");
  CHECK(s.ui.settingsStore().writes == 1);
  s.rotate(3);
  s.click();                 // Motor dir
  EXPECT_LINE(s, 7, "?Motor dir Reverse");

  s.send("{\"type\":\"reset\"}");
  s.tick();
  EXPECT_LINE(s, 0, BOOT_TITLE);
  s.boot();
  openMain(s, 4);
  EXPECT_LINE(s, 2, "Flash: loaded");
  EXPECT_LINE(s, 3, "RAduty     +40.0 PWM");
  EXPECT_LINE(s, 7, " Motor dir Normal");   // not saved: only RAduty was

  s.rotate(6);
  s.click();                 // Reset defaults?
  CHECK(std::strcmp(s.line(7) + 1, "Reset? click again") == 0);   // literal "?" : skip the cursor only
  s.click();                 // confirmed
  s.tick();
  s.rotate(-10);
  EXPECT_LINE(s, 2, "Flash: saved");
  EXPECT_LINE(s, 3, "RAduty      +0.0 PWM");
}

void test_sleep_and_health() {
  Sim s;
  s.boot();
  s.rotate(6);
  s.click();
  EXPECT_SCREEN(s, {" ", " ", " ", " ", " ", " ", " "});
  EXPECT_LINE(s, 0, "T 12: 0: 0.0 V 12.60");   // status bar stays
  s.click();
  EXPECT_LINE(s, 7, "?Sleep");
  openMain(s, 2);
  s.rotate(5);
  s.click();
  EXPECT_SCREEN(s, {"*Health", "Loop 100000/ 10.0Hz", "MAIN  24300", "DISP  21800",
                    "GPS     650", "IMU    1250", "EST     180 AST  4000"});
}

void test_phone_time_power_and_wifi() {
  Sim s;
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":0,\"sats\":0}}");
  s.boot();
  EXPECT_LINE(s, 0, "T 12: 0: 0.0 V 12.60");   // no fix, no clock: GPS fields as they are
  s.send("{\"type\":\"sensors\",\"power\":{\"v\":11.84},\"remote\":{\"state\":1}}");
  s.tick();
  EXPECT_LINE(s, 0, "T 12: 0: 0.0 V 11.84w");
  // page connects and sends its clock (2026-11-25 03:04:05.6 UTC)
  s.send("{\"type\":\"sensors\",\"remote\":{\"state\":2}}");
  s.send("{\"type\":\"phone_time\",\"unix_s\":1795575845,\"ms\":600}");
  s.tick();
  EXPECT_LINE(s, 0, "P  3: 4: 5.6 V 11.84W");
  s.tick(10);                                  // runs on with the frames
  EXPECT_LINE(s, 0, "P  3: 4: 6.6 V 11.84W");
  openMain(s, 2);                              // Status > GPS shows the source
  s.click();
  EXPECT_LINE(s, 5, "Pos ---  Time PHN");
  s.send("{\"type\":\"sensors\",\"gps\":{\"fix\":1,\"sats\":8}}");
  s.tick();
  EXPECT_LINE(s, 5, "Pos GPS  Time GPS");      // GPS fix wins again
}

void test_protocol_messages() {
  Sim s;
  s.send("{\"type\":\"hello\"}");
  CHECK(s.replies.size() == 1 && s.replies[0].find("\"backend\":\"firmware\"") != std::string::npos);
  s.send("{not json");
  CHECK(s.replies.size() == 1 && s.replies[0].find("\"type\":\"error\"") != std::string::npos);
  s.send("{\"type\":\"bogus\"}");
  CHECK(s.replies.size() == 1 && s.replies[0].find("unknown type") != std::string::npos);
  s.send("");
  CHECK(s.replies.empty());
  s.tick(3);
  CHECK(s.replies.size() == 3);
  CHECK(s.replies[2].find("\"seq\":2,\"t_ms\":200") != std::string::npos);
  CHECK(s.replies[0].find("\"lines\":[\"" BOOT_TITLE "\",\"\"") != std::string::npos);
  std::vector<std::string> r;
  CHECK(s.session.handleLine("{\"type\":\"quit\"}", r) == false);
}

}  // namespace

int main() {
  struct { const char* name; void (*fn)(); } tests[] = {
#define T(f) {#f, f}
    T(test_boot_then_main_menu), T(test_boot_failures), T(test_input_ignored_during_boot),
    T(test_reset_restarts_ui), T(test_ra_speed_edit_drives_motor),
    T(test_gps_screen_shows_sensor_values), T(test_last_gps_fix_is_kept), T(test_last_gps_needs_good_hdop),
    T(test_imu_screens),
    T(test_moon_target), T(test_gyro_bias_calibration),
    T(test_att_estimator_converges), T(test_att_estimator_uses_gyro_bias), T(test_polar_align),
    T(test_target_tracking), T(test_settings_survive_reset), T(test_sleep_and_health),
    T(test_phone_time_power_and_wifi), T(test_protocol_messages),
#undef T
  };
  for (auto& t : tests) {
    g_test = t.name;
    int before = g_failures;
    t.fn();
    std::printf("%s %s\n", g_failures == before ? "ok  " : "FAIL", t.name);
  }
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
