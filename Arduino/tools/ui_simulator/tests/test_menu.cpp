// Menu logic tests for the runner: input sequence -> expected screen lines.
// Drives sim::Session (the same code path the GUI uses), so these also cover
// the protocol layer. Build and run with ../run_tests.sh.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mock_ui.h"
#include "protocol.h"

namespace {

int g_checks = 0;
int g_failures = 0;
const char* g_test = "";

#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failures++; \
  std::printf("  FAIL [%s] %s:%d  %s\n", g_test, __FILE__, __LINE__, #cond); } } while (0)

// Test harness around a Session with the mock backend.
std::string num(int v) { char b[16]; std::snprintf(b, sizeof(b), "%d", v); return b; }

struct Sim {
  sim::MockUi ui;
  sim::Session session{&ui};
  std::vector<std::string> replies;

  void send(const std::string& msg) {
    replies.clear();
    session.handleLine(msg, replies);
  }
  void tick(int n = 1) { send("{\"type\":\"tick\",\"n\":" + num(n) + "}"); }
  void rotate(int n) { send("{\"type\":\"input\",\"rotate\":" + num(n) + "}"); tick(); }
  void click() { send("{\"type\":\"input\",\"click\":1}"); tick(); }
  void boot() { tick(sim::MockUi::kBootLineFrames * 8 + sim::MockUi::kBootHoldFrames + 1); }
  const char* line(int i) const { return session.lastFrame().lines[i]; }
};

// '?' in the pattern matches the blinking cursor ('>' or '-').
bool lineMatches(const char* got, const char* pattern) {
  size_t n = std::strlen(pattern);
  if (std::strlen(got) != n) return false;
  for (size_t i = 0; i < n; i++) {
    if (pattern[i] == '?') { if (got[i] != '>' && got[i] != '-') return false; }
    else if (pattern[i] != got[i]) return false;
  }
  return true;
}

void expectLine(Sim& s, int i, const char* pattern, int src_line) {
  g_checks++;
  if (!lineMatches(s.line(i), pattern)) {
    g_failures++;
    std::printf("  FAIL [%s] line %d of test (%d): got \"%s\" expected \"%s\"\n",
                g_test, i, src_line, s.line(i), pattern);
  }
}
#define EXPECT_LINE(s, i, pat) expectLine(s, i, pat, __LINE__)

// Expect the content area (lines 1..7) exactly.
void expectScreen(Sim& s, const char* const (&pat)[7], int src_line) {
  for (int i = 0; i < 7; i++) expectLine(s, i + 1, pat[i], src_line);
}
#define EXPECT_SCREEN(s, ...) do { const char* const _p[7] = __VA_ARGS__; expectScreen(s, _p, __LINE__); } while (0)

/******************************************************************/

void test_boot_then_main_menu() {
  Sim s;
  s.tick();
  EXPECT_LINE(s, 0, "1. Display is ON");
  EXPECT_LINE(s, 1, "");
  s.tick(sim::MockUi::kBootLineFrames * 7);
  EXPECT_LINE(s, 1, "2. IMU is ON");
  EXPECT_LINE(s, 5, "3. GPS is ON");
  EXPECT_LINE(s, 7, "5. GPIO/PWM is ON");
  CHECK(s.ui.screen() == sim::MockUi::kBoot);
  CHECK(s.session.lastMotor().pwm == 120);

  s.tick(sim::MockUi::kBootHoldFrames - 1);
  CHECK(s.ui.screen() == sim::MockUi::kBoot);   // boot log is still held
  s.tick();
  CHECK(s.ui.screen() == sim::MockUi::kMain);
  EXPECT_LINE(s, 0, "T 12:00:00.0 V12.60");
  EXPECT_SCREEN(s, {"Main Menu", "?1RA Speed", " 2Status-GPS", " 3Status-Acc/Gyro",
                    " 4Status-Mag/Alt", " 5Status-IMUCalib", " 6Do IMU Calib."});
}

void test_boot_failures() {
  Sim s;
  s.send("{\"type\":\"reset\",\"imu_ok\":false,\"gps_ok\":false}");
  s.tick(sim::MockUi::kBootLineFrames * 8);
  EXPECT_LINE(s, 1, ">> IMU failed");
  EXPECT_LINE(s, 5, ">> GPS is timeout");
}

void test_input_ignored_during_boot() {
  Sim s;
  s.send("{\"type\":\"input\",\"rotate\":3,\"click\":1}");
  s.boot();
  EXPECT_LINE(s, 2, "?1RA Speed");
}

void test_cursor_blinks() {
  Sim s;
  s.boot();
  char a = s.line(2)[0];
  s.tick();
  char b = s.line(2)[0];
  CHECK(a != b);
  CHECK((a == '>' && b == '-') || (a == '-' && b == '>'));
}

void test_main_menu_scrolls() {
  Sim s;
  s.boot();
  s.rotate(5);   // cursor reaches the last visible row
  EXPECT_SCREEN(s, {"Main Menu", " 1RA Speed", " 2Status-GPS", " 3Status-Acc/Gyro",
                    " 4Status-Mag/Alt", " 5Status-IMUCalib", "?6Do IMU Calib."});
  s.rotate(1);   // one more step scrolls the list by one line
  EXPECT_SCREEN(s, {"Main Menu", " 2Status-GPS", " 3Status-Acc/Gyro", " 4Status-Mag/Alt",
                    " 5Status-IMUCalib", " 6Do IMU Calib.", "?7Att.Estimator"});
  s.rotate(10);  // clamps at the last item
  EXPECT_SCREEN(s, {"Main Menu", " 5Status-IMUCalib", " 6Do IMU Calib.", " 7Att.Estimator",
                    " 8Health", " 9Axis for Moon", "?Sleep"});
  s.rotate(-5);  // back up to the top visible row, no scroll yet
  EXPECT_LINE(s, 2, "?5Status-IMUCalib");
  s.rotate(-1);
  EXPECT_LINE(s, 2, "?4Status-Mag/Alt");
  s.rotate(-20);
  EXPECT_LINE(s, 2, "?1RA Speed");
}

// Every main menu item can be entered and left; the cursor comes back to it.
void test_enter_and_leave_every_screen() {
  struct Item { const char* title; int exit_rotate; };   // rotations to reach Exit/Quit
  const Item items[10] = {
    {"*1RA Speed", 2}, {"*2Status-GPS", -1}, {"*3Status-Acc/Gyro", -1},
    {"*4Status-Mag/Alt", -1}, {"*5Status-IMUCalib", -1}, {"*6Do IMU Calib.", 4},
    {"*7Att.Estimator", 12}, {"*8Health", -1}, {"*9Axis for Moon", -1}, {"(Sleep)", -1},
  };
  const char* names[10] = {"1RA Speed", "2Status-GPS", "3Status-Acc/Gyro", "4Status-Mag/Alt",
                           "5Status-IMUCalib", "6Do IMU Calib.", "7Att.Estimator", "8Health",
                           "9Axis for Moon", "Sleep"};
  for (int k = 0; k < 10; k++) {
    Sim s;
    s.boot();
    s.rotate(k);
    s.click();
    EXPECT_LINE(s, 1, items[k].title);
    EXPECT_LINE(s, 0, "T 12:00:00.0 V12.60");
    if (items[k].exit_rotate >= 0) {
      s.rotate(items[k].exit_rotate);
      s.click();
    } else {
      s.rotate(3);   // info screens ignore rotation
      EXPECT_LINE(s, 1, items[k].title);
      s.click();
    }
    CHECK(s.ui.screen() == sim::MockUi::kMain);
    EXPECT_LINE(s, 1, "Main Menu");
    // cursor is back on item k (row depends on scroll)
    bool found = false;
    for (int r = 2; r < 8; r++) {
      std::string pat = std::string("?") + names[k];
      if (lineMatches(s.line(r), pat.c_str())) found = true;
    }
    g_checks++;
    if (!found) { g_failures++; std::printf("  FAIL [%s] cursor not on item %d\n", g_test, k); }
  }
}

void test_ra_speed_edit_drives_motor() {
  Sim s;
  s.boot();
  s.click();
  EXPECT_SCREEN(s, {"*1RA Speed", "?RRcmd    0.0000DEG", " RRate    0.0000DEG", " Exit",
                    " ROL    0.0000   DEG", " LAT N 37.566500DEG", " PIT    0.0000   DEG"});
  s.click();   // RRcmd -> edit mode
  EXPECT_LINE(s, 1, "*Set RA Speed");
  s.rotate(5);
  EXPECT_LINE(s, 2, "?RRcmd    5.0000DEG");
  CHECK(s.session.lastMotor().in1 == 0);
  CHECK(s.session.lastMotor().in2 == 1);
  CHECK(s.session.lastMotor().pwm == 5);
  s.rotate(-12);
  EXPECT_LINE(s, 2, "?RRcmd   -7.0000DEG");
  CHECK(s.session.lastMotor().in1 == 1);
  CHECK(s.session.lastMotor().in2 == 0);
  CHECK(s.session.lastMotor().pwm == 7);
  s.rotate(7);
  CHECK(s.session.lastMotor().in1 == 0 && s.session.lastMotor().in2 == 0);
  s.rotate(300);
  CHECK(s.session.lastMotor().pwm == 255);
  s.click();   // confirm
  EXPECT_LINE(s, 1, "*1RA Speed");
  s.rotate(1);   // RRate is read-only
  s.click();
  EXPECT_LINE(s, 1, "*1RA Speed");
  EXPECT_LINE(s, 3, "?RRate    0.0000DEG");
  s.rotate(1);
  s.click();   // Exit
  EXPECT_LINE(s, 1, "Main Menu");
  CHECK(s.session.lastMotor().pwm == 255);   // motor keeps running after leaving
}

void test_gps_screen_shows_sensor_values() {
  Sim s;
  s.boot();
  s.send("{\"type\":\"sensors\",\"gps\":{\"year\":26,\"month\":3,\"day\":9,\"hour\":21,"
         "\"minute\":5,\"seconds\":7,\"milliseconds\":450,\"lat\":-33.5,\"lon\":-70.25,"
         "\"fix\":2,\"sats\":11}}");
  s.rotate(1);
  s.click();
  EXPECT_LINE(s, 0, "T 21:05:07.4 V12.60");
  EXPECT_SCREEN(s, {"*2Status-GPS", "Date 2026/03/09", "Time   21:05:07.450", "DGPS  Sat 11",
                    "", " S  33.50000000 DEG", " W  70.25000000 DEG"});
}

void test_imu_screens() {
  Sim s;
  s.boot();
  s.send("{\"type\":\"sensors\",\"imu\":{\"ax\":-4.903325,\"ay\":0,\"az\":8.492806,"
         "\"wx\":1.25,\"wy\":-0.5,\"wz\":15,\"mx\":0,\"my\":-30,\"mz\":-40,\"temp\":25.5}}");
  s.rotate(2);
  s.click();
  EXPECT_SCREEN(s, {"*3Status-Acc/Gyro", "Ax      -4.9033 mpss", "Ay       0.0000 mpss",
                    "Az       8.4928 mpss", "Wx       1.2500 dps", "Wy      -0.5000 dps",
                    "Wz      15.0000 dps"});
  s.click();
  s.rotate(1);
  s.click();
  EXPECT_SCREEN(s, {"*4Status-Mag/Alt", "Mx       0.0000 uT", "My     -30.0000 uT",
                    "Mz     -40.0000 uT", "HEAD    90.0000 DEG", "Temp    25.5000 C", ""});
  s.click();
  s.rotate(5);   // 9 Axis for Moon
  s.click();
  EXPECT_LINE(s, 3, "Pitch   30.0000 DEG");
}

void test_do_imu_calib_menu() {
  Sim s;
  s.boot();
  s.rotate(5);
  s.click();
  EXPECT_SCREEN(s, {"*6Do IMU Calib.", "?Acc. Bias Calib", " Acc. Scale Calib",
                    " Gyro Bias Calib", " Gyro Scale Calib", "", " Quit"});
  s.rotate(1);
  s.click();
  EXPECT_LINE(s, 1, "*Acc. Scale Calib");
  s.click();
  EXPECT_LINE(s, 3, "?Acc. Scale Calib");   // cursor remembered
  s.rotate(3);                              // skips the blank row
  EXPECT_LINE(s, 7, "?Quit");
  s.rotate(-4);
  s.click();
  EXPECT_LINE(s, 1, "*Acc. Bias Calib");
  s.click();
  s.rotate(3);
  s.click();
  EXPECT_LINE(s, 1, "*Gyro Scale Calib");
  s.click();
  s.rotate(1);
  s.click();   // Quit
  EXPECT_LINE(s, 1, "Main Menu");
  EXPECT_LINE(s, 7, "?6Do IMU Calib.");
}

void test_gyro_bias_calibration() {
  Sim s;
  s.boot();
  s.send("{\"type\":\"sensors\",\"imu\":{\"wx\":0.5,\"wy\":-0.25,\"wz\":0.125}}");
  s.rotate(5);
  s.click();
  s.rotate(2);
  s.click();
  EXPECT_LINE(s, 1, "Calib Gyro Bias Ready");
  s.tick(10);
  EXPECT_LINE(s, 1, "Calib Gyro Bias ing.1");
  s.tick(50);
  EXPECT_LINE(s, 1, "Calib Gyro Bias ing.2");
  s.tick(100);
  EXPECT_LINE(s, 1, "Calib Gyro Bias Done.");
  EXPECT_LINE(s, 5, "bWx      0.5000 dps");
  EXPECT_LINE(s, 2, "Wx  0.50000-> 0.00000");
  s.click();
  EXPECT_LINE(s, 4, "?Gyro Bias Calib");
  s.rotate(2);
  s.click();   // Quit
  s.rotate(-1);
  s.click();   // 5 Status-IMUCalib shows the new bias
  EXPECT_LINE(s, 1, "*5Status-IMUCalib");
  EXPECT_LINE(s, 6, "bWy     -0.2500 dps");
}

void test_att_estimator_scroll() {
  Sim s;
  s.boot();
  s.rotate(6);
  s.click();
  EXPECT_SCREEN(s, {"*7Att.Estimator", "?Roll     0.0000DEG", " PITCH    0.0000DEG",
                    " YAW      0.0000DEG", " R_R      0.5000", " R_P      0.5000",
                    " R_Y      2.0000"});
  s.click();   // not Exit: stays
  EXPECT_LINE(s, 1, "*7Att.Estimator");
  s.rotate(12);
  EXPECT_SCREEN(s, {"*7Att.Estimator", " Q_P      0.0010", " Q_Y      0.0100", " K_R      0.0447",
                    " K_P      0.0447", " K_Y      0.0707", "?Exit"});
}

void test_protocol_messages() {
  Sim s;
  s.send("{\"type\":\"hello\"}");
  CHECK(s.replies.size() == 1 && s.replies[0].find("\"backend\":\"mock\"") != std::string::npos);
  s.send("{not json");
  CHECK(s.replies.size() == 1 && s.replies[0].find("\"type\":\"error\"") != std::string::npos);
  s.send("{\"type\":\"bogus\"}");
  CHECK(s.replies.size() == 1 && s.replies[0].find("unknown type") != std::string::npos);
  s.send("");
  CHECK(s.replies.empty());
  s.tick(3);
  CHECK(s.replies.size() == 3);
  CHECK(s.replies[2].find("\"seq\":2,\"t_ms\":200") != std::string::npos);
  CHECK(s.replies[0].find("\"lines\":[\"1. Display is ON\",\"\"") != std::string::npos);
  std::vector<std::string> r;
  CHECK(s.session.handleLine("{\"type\":\"quit\"}", r) == false);
}

}  // namespace

int main() {
  struct { const char* name; void (*fn)(); } tests[] = {
#define T(f) {#f, f}
    T(test_boot_then_main_menu), T(test_boot_failures), T(test_input_ignored_during_boot),
    T(test_cursor_blinks), T(test_main_menu_scrolls), T(test_enter_and_leave_every_screen),
    T(test_ra_speed_edit_drives_motor), T(test_gps_screen_shows_sensor_values),
    T(test_imu_screens), T(test_do_imu_calib_menu), T(test_gyro_bias_calibration),
    T(test_att_estimator_scroll), T(test_protocol_messages),
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
