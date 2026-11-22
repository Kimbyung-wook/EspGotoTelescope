#include "test_main.h"
#include "../app/src/ui/telescope_ui.h"

namespace {
struct UiFixture {
  TelescopeUi ui;
  UiContext ctx;
  Frame f;
  void step(int delta = 0, bool click = false) { f.clear(); ui.step(delta, click, ctx, f); }
  // Find the content row whose text after the cursor column equals label
  int rowOf(const char* label) {
    for (int r = 2; r < 8; r++) if (std::strcmp(f.lines[r] + 1, label) == 0) return r;
    return -1;
  }
  char cursorAt(int row) { return f.lines[row][0]; }
};
}

TEST(ui_status_bar_format) {
  UiFixture u;
  u.ctx.tel.hour = 21; u.ctx.tel.minute = 3; u.ctx.tel.seconds = 7; u.ctx.tel.milliseconds = 450;
  u.step();
  CHECK_STR(u.f.lines[0], "T 21: 3: 7.4 V 12.60");
}

TEST(ui_main_menu_first_page_and_blink) {
  UiFixture u;
  u.step();
  CHECK_STR(u.f.lines[1], "Main Menu");
  CHECK_STR(u.f.lines[2], ">1RA Speed");
  CHECK_STR(u.f.lines[3], " 2Status - GPS");
  CHECK_STR(u.f.lines[7], " 6Do IMU Calib.");
  u.step();
  CHECK_STR(u.f.lines[2], "-1RA Speed");      // cursor blinks
}

TEST(ui_main_menu_scrolls_one_item_per_step) {
  UiFixture u;
  u.step(5);                                     // item 6 : last row, no scroll
  CHECK_STR(u.f.lines[2] + 1, "1RA Speed");
  CHECK(u.cursorAt(7) != ' ');
  u.step(1);                                     // item 7 : scrolled by one
  CHECK_STR(u.f.lines[2] + 1, "2Status - GPS");
  CHECK_STR(u.f.lines[7] + 1, "7Att.Estimator");
  CHECK(u.cursorAt(7) != ' ');
  u.step(4);                                     // item 11 (Sleep) reachable
  CHECK_STR(u.f.lines[7] + 1, "Sleep");
  CHECK(u.cursorAt(7) != ' ');
}

TEST(ui_main_menu_clamps_at_both_ends) {
  UiFixture u;
  u.step(-3);
  CHECK(u.ui.mainMenu().selected() == 0);
  u.step(50);
  CHECK(u.ui.mainMenu().selected() == 10);
  u.step(-1);
  CHECK(u.ui.mainMenu().selected() == 9);       // one step back from the end
}

TEST(ui_every_menu_item_opens_and_returns_to_same_item) {
  for (int item = 0; item < MainMenuScreen::ITEMS; item++) {
    UiFixture u;
    u.step(item);
    u.step(0, true);                             // enter
    CHECK(u.ui.manager().depth() == 2);
    if (item == 0) { u.step(2); }                // RA Speed : move to Quit
    if (item == 5) { u.step(5); }                // Do IMU Calib : move to Quit
    if (item == 9) { u.step(5); }                // Planet Tracking : Mercury -> Quit
    u.step(0, true);                             // back
    CHECK(u.ui.manager().depth() == 1);
    CHECK(u.ui.mainMenu().selected() == item);
  }
}

TEST(ui_info_screen_ignores_rotation) {
  UiFixture u;
  u.step(1); u.step(0, true);                    // Status - GPS
  CHECK_STR(u.f.lines[1], "*2Status - GPS");
  u.step(4);
  u.step(0, true);                               // back
  CHECK(u.ui.mainMenu().selected() == 1);
}

TEST(ui_gps_screen_lines) {
  UiFixture u;
  Telemetry& t = u.ctx.tel;
  t.year = 26; t.month = 11; t.day = 21; t.hour = 9; t.minute = 5; t.seconds = 1; t.milliseconds = 200;
  t.fixquality = 1; t.satellites = 8; t.lat_hemi = 'N'; t.latitude = 37.5; t.lon_hemi = 'E'; t.longitude = 127.25;
  u.step(1); u.step(0, true);
  CHECK_STR(u.f.lines[2], "Date: 2026/11/21");
  CHECK_STR(u.f.lines[3], "Time:   09:05:01.200");
  CHECK_STR(u.f.lines[4], "  GPS Sat  8");
  CHECK_STR(u.f.lines[6], " N  37.50000000 DEG");
  CHECK_STR(u.f.lines[7], " E 127.25000000 DEG");
}

TEST(ui_ra_speed_edit_changes_command) {
  UiFixture u;
  u.step(0, true);                               // RA Speed
  CHECK_STR(u.f.lines[1], "*1RA Speed");
  u.step(0, true);                               // edit RRcmd
  CHECK(u.ui.raSpeed().editing());
  u.step(+5);
  CHECK_NEAR(u.ctx.state.ra_speed_cmd, 5.0, 1e-6);
  CHECK_STR(u.f.lines[1], "*Set RA Speed");
  u.step(-8);
  CHECK_NEAR(u.ctx.state.ra_speed_cmd, -3.0, 1e-6);
  CHECK(std::strstr(u.f.lines[2], "-3.00000DEG") != nullptr);
  u.step(0, true);                               // finish editing
  CHECK(!u.ui.raSpeed().editing());
  u.step(+1);                                    // now moves the cursor, not the speed
  CHECK_NEAR(u.ctx.state.ra_speed_cmd, -3.0, 1e-6);
  CHECK(u.f.lines[3][0] != ' ');                 // cursor on RRate
}

TEST(ui_ra_speed_rrate_click_does_nothing) {
  UiFixture u;
  u.step(0, true); u.step(1); u.step(0, true);
  CHECK(u.ui.manager().depth() == 2);
  CHECK(!u.ui.raSpeed().editing());
}

TEST(ui_gyro_bias_calibration_phases) {
  UiFixture u;
  u.ctx.tel.imu.wx = 0.5f; u.ctx.tel.imu.wy = -0.2f; u.ctx.tel.imu.wz = 0.1f;
  u.step(5); u.step(0, true);                    // Do IMU Calib.
  CHECK_STR(u.f.lines[1], "*6Do IMU Calib.");
  u.step(2); u.step(0, true);                    // Gyro Bias Calib
  CHECK_STR(u.f.lines[1], "Calib Gyro Bias Ready");
  for (unsigned long ms = 100; ms <= 16000; ms += 100) { u.ctx.tel.now_ms = ms; u.step(); }
  CHECK_STR(u.f.lines[1], "Calib Gyro Bias Done.");
  CHECK_NEAR(u.ctx.state.gyro_bias[0], 0.5, 1e-4);
  CHECK_NEAR(u.ctx.state.gyro_bias[1], -0.2, 1e-4);
  u.step(0, true);                               // back to the calib menu, cursor on Gyro Bias
  CHECK(u.ui.manager().depth() == 2);
  CHECK(u.f.lines[4][0] != ' ');
  u.step(0, false);
  u.step(3); u.step(0, true);                    // Quit
  CHECK(u.ui.manager().depth() == 1);
  CHECK(u.ui.mainMenu().selected() == 5);
}

TEST(ui_lines_never_overflow) {
  UiFixture u;
  u.ctx.tel.imu.ax = -123456.0f; u.ctx.tel.latitude = -99999.0; u.ctx.state.ra_speed_cmd = -1e9f;
  for (int item = 0; item < MainMenuScreen::ITEMS; item++) {
    u.step(-20); u.step(item); u.step(0, true); u.step();
    for (int r = 0; r < 8; r++) CHECK(std::strlen(u.f.lines[r]) <= 21);
    if (item == 0) u.step(2);
    if (item == 5) u.step(5);
    if (item == 9) u.step(5);
    u.step(0, true);
  }
}

TEST(ui_planet_tracking_select_and_toggle) {
  UiFixture u;
  u.step(9); u.step(0, true);                    // 10Planet Tracking
  CHECK_STR(u.f.lines[1], "*10Planet Tracking");
  CHECK(u.cursorAt(3) != ' ');                   // starts on Mercury (row 3)
  CHECK_STR(u.f.lines[2] + 1, "Moon");
  u.step(2); u.step(0, true);                    // Mars
  CHECK(u.ctx.state.track_target == 3);
  CHECK_STR(u.f.lines[1], "*Mars");
  CHECK_STR(u.f.lines[4], "   no GPS fix");      // no telemetry yet
  u.step(0, true);                               // Track on
  CHECK(u.ctx.state.tracking);
  CHECK_STR(u.f.lines[7], " Back   (RA uncal)");
  u.ctx.tel.target_valid = true; u.ctx.tel.target_index = 3;
  u.ctx.tel.target_ra_deg = 151.48; u.ctx.tel.target_dec_deg = 13.93;
  u.ctx.tel.target_alt_deg = -27.89; u.ctx.tel.target_az_deg = 41.35;
  u.ctx.tel.target_ha_deg = 123.456; u.ctx.tel.target_dist = 1.2515;
  u.ctx.tel.target_rate_arcsec_s = 15.0236; u.ctx.tel.ra_calibrated = true;
  u.step();
  CHECK_STR(u.f.lines[2], "R.A.   151.48000 DEG");
  CHECK_STR(u.f.lines[4], "ALT -27.89 AZ  41.35");
  CHECK_STR(u.f.lines[5], "HA  123.456 D  1.2515");
  CHECK_STR(u.f.lines[7], " Back");
  CHECK(u.f.lines[6][0] != ' ');
  CHECK_STR(u.f.lines[6] + 1, "Track ON  15.0236\"/s");
  u.step(1); u.step(0, true);                    // Back : tracking stays on
  CHECK(u.ctx.state.tracking);
  CHECK_STR(u.f.lines[1], "*10Planet Tracking");
  CHECK_STR(u.f.lines[5] + 1, "Mars     TRACKING");
  u.step(1); u.step(0, true);                    // Jupiter : a new target starts stopped
  CHECK(u.ctx.state.track_target == 4);
  CHECK(!u.ctx.state.tracking);
}

TEST(ui_manual_ra_speed_stops_tracking) {
  UiFixture u;
  u.ctx.state.tracking = true;
  u.step(0, true);                               // 1RA Speed
  u.step(0, true);                               // edit RRcmd
  CHECK(!u.ctx.state.tracking);
}
