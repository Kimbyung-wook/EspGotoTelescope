// Unit tests of the UI screens: one screen at a time, Telemetry/UiState set by hand,
// checks of the frame lines and of UiState (requests, motor_inhibit, cfg).
// The whole loop (sensors -> Core -> screen -> motor, boot, protocol, flash) is
// tested by the simulator (tools/ui_simulator/tests/test_menu.cpp), not here.
#include "test_main.h"
#include "../app/src/ui/simplegoto_ui.h"
#include "../app/src/location/location.h"
#include "../app/src/astro/astro.h"

namespace {
struct UiFixture {
  SimpleGoToUi ui;
  UiContext ctx;
  Frame f;
  UiFixture() { ctx.tel.power.valid = true; ctx.tel.power.volts = 12.6f; }   // as measured
  void step(int delta = 0, bool click = false) {
    f.clear();
    ui.step(delta, click, ctx, f);
    CHECK(!f.overflow);                           // no line is cut at 21 characters
    if (f.overflow) for (int r = 0; r < Frame::LINES; r++) std::printf("    |%s|\n", f.lines[r]);
  }
  void click() { step(0, true); }
  void open(int main_item) { step(main_item); click(); }
  // Run the clock forward, one frame per 100 ms
  void runTo(unsigned long ms) {
    while (ctx.tel.now_ms < ms) { ctx.tel.now_ms += 100; step(); }
  }
  char cursorAt(int row) { return f.lines[row][0]; }
  bool has(int row, const char* text) { return std::strstr(f.lines[row], text) != nullptr; }
};
// Position and time known, as app::updateLocationTime() leaves them
void known(Telemetry& t, double lat = 37.5) {
  t.obs.loc_source = loc::SRC_GPS; t.obs.time_source = loc::TIME_GPS;
  t.obs.lat_deg = lat; t.obs.lon_deg = 127.0;
}
}

/************************************/
// Status bar, main menu
/************************************/
TEST(ui_status_bar_format) {
  UiFixture u;
  u.ctx.tel.obs.time_source = loc::TIME_GPS;
  u.ctx.tel.obs.clock_hour = 21; u.ctx.tel.obs.clock_minute = 3; u.ctx.tel.obs.clock_second = 7; u.ctx.tel.obs.clock_ms = 450;
  u.step();
  CHECK_STR(u.f.lines[0], "T 21: 3: 7.4 V 12.60");
  u.ctx.tel.obs.time_source = loc::TIME_MANUAL;       // manual clock (DES-LOC-04)
  u.step();
  CHECK_STR(u.f.lines[0], "M 21: 3: 7.4 V 12.60");
  u.ctx.tel.obs.time_source = loc::TIME_PHONE;        // phone clock (DES-LOC-05)
  u.step();
  CHECK_STR(u.f.lines[0], "P 21: 3: 7.4 V 12.60");
}

TEST(ui_status_bar_voltage) {
  UiFixture u;
  u.ctx.tel.power.valid = false;                // A0 not wired / not read yet
  u.ctx.tel.remote = Telemetry::REMOTE_CONNECTED;
  u.step();
  CHECK_STR(u.f.lines[0], "T  0: 0: 0.0 V --.--W");
  u.ctx.tel.power.valid = true;
  u.ctx.tel.power.volts = 9.876f;
  u.step();
  CHECK_STR(u.f.lines[0], "T  0: 0: 0.0 V  9.88W");
  u.ctx.tel.power.volts = 123.0f;                     // clamped: the line stays 20 + 1 characters
  u.step();
  CHECK_STR(u.f.lines[0], "T  0: 0: 0.0 V 99.99W");
}

TEST(ui_sleep_turns_the_display_off) {
  UiFixture u;
  u.step();
  CHECK(!u.ctx.state.display_off);
  u.open(6);                                      // Sleep
  CHECK(u.ctx.state.display_off);
  u.step(3);                                      // rotation keeps it asleep
  CHECK(u.ctx.state.display_off);
  u.click();                                      // wake
  CHECK(!u.ctx.state.display_off);
  CHECK_STR(u.f.lines[1], "Main Menu");
}

TEST(ui_status_bar_wifi_mark) {
  UiFixture u;
  u.ctx.tel.obs.clock_hour = 21; u.ctx.tel.obs.clock_minute = 3; u.ctx.tel.obs.clock_second = 7; u.ctx.tel.obs.clock_ms = 450;
  u.ctx.tel.remote = Telemetry::REMOTE_WAITING;     // AP on, no phone
  u.step();
  CHECK_STR(u.f.lines[0], "T 21: 3: 7.4 V 12.60w");
  u.ctx.tel.remote = Telemetry::REMOTE_CONNECTED;   // phone connected
  u.step();
  CHECK_STR(u.f.lines[0], "T 21: 3: 7.4 V 12.60W");
  u.ctx.tel.remote = Telemetry::REMOTE_OFF;
  u.step();
  CHECK_STR(u.f.lines[0], "T 21: 3: 7.4 V 12.60");
}

TEST(ui_main_menu_items_and_blink) {
  UiFixture u;
  u.step();
  CHECK_STR(u.f.lines[1], "Main Menu");
  CHECK_STR(u.f.lines[2], ">1Target");
  CHECK_STR(u.f.lines[3], " 2Polar Align");
  CHECK_STR(u.f.lines[4], " 3Status");
  CHECK_STR(u.f.lines[5], " 4Calibration");
  CHECK_STR(u.f.lines[6], " 5Settings");
  CHECK_STR(u.f.lines[7], " 6RA Speed");
  u.step();
  CHECK_STR(u.f.lines[2], "-1Target");           // cursor blinks
  u.step(6);                                     // Sleep : scrolled by one
  CHECK_STR(u.f.lines[2] + 1, "2Polar Align");
  CHECK_STR(u.f.lines[7] + 1, "Sleep");
  CHECK(u.cursorAt(7) != ' ');
  u.step(-1);                                    // back up one: the window returns to the top
  CHECK_STR(u.f.lines[2], " 1Target");
  CHECK(u.cursorAt(7) != ' ' && u.has(7, "6RA Speed"));
  u.step(-20);
  CHECK(u.cursorAt(2) != ' ' && u.has(2, "1Target"));
}

TEST(ui_main_menu_clamps_at_both_ends) {
  UiFixture u;
  u.step(-3);
  CHECK(u.ui.mainMenu().selected() == 0);
  u.step(50);
  CHECK(u.ui.mainMenu().selected() == SimpleGoToUi::MAIN_ITEMS - 1);
  u.step(-1);
  CHECK(u.ui.mainMenu().selected() == SimpleGoToUi::MAIN_ITEMS - 2);
}

// Rotations from the first item of each main screen to its Back/Quit
static const char* const TITLE[SimpleGoToUi::MAIN_ITEMS] = {
  "*1Target", "*2Polar Align", "*3Status", "*4Calibration", "*5Settings", "*6RA Speed", " ",
};
static const char* const ITEM[SimpleGoToUi::MAIN_ITEMS] = {
  "1Target", "2Polar Align", "3Status", "4Calibration", "5Settings", "6RA Speed", "Sleep",
};
static const int TO_BACK[SimpleGoToUi::MAIN_ITEMS] = {
  5,   // Target menu : starts on Mercury, Quit is item 6
  1,   // Polar Align : Confirm, Back
  6,   // Status : 6 screens, Back
  2,   // Calibration : Gyro, Accel, Back
  7,   // Settings : 8 items
  2,   // RA Speed : RRcmd, RRate, Quit
  0,   // Sleep : click wakes
};

TEST(ui_every_menu_item_opens_and_returns_to_same_item) {
  for (int item = 0; item < SimpleGoToUi::MAIN_ITEMS; item++) {
    UiFixture u;
    u.open(item);
    CHECK_STR(u.f.lines[1], TITLE[item]);
    CHECK(u.ui.manager().depth() == 2);
    u.step(TO_BACK[item]);
    if (item == SimpleGoToUi::MAIN_ITEMS - 1) { u.step(3); CHECK_STR(u.f.lines[1], " "); }   // Sleep ignores the dial
    u.click();                                   // back
    CHECK(u.ui.manager().depth() == 1);
    CHECK_STR(u.f.lines[1], "Main Menu");
    CHECK(u.ui.mainMenu().selected() == item);
    bool drawn = false;                          // the cursor is drawn on that item
    for (int r = 2; r < Frame::LINES; r++) if (u.cursorAt(r) != ' ' && std::strcmp(u.f.lines[r] + 1, ITEM[item]) == 0) drawn = true;
    CHECK(drawn);
  }
}

TEST(ui_status_menu_and_info_screens) {
  UiFixture u;
  Telemetry& t = u.ctx.tel;
  t.gps.year = 26; t.gps.month = 11; t.gps.day = 21; t.gps.hour = 9; t.gps.minute = 5; t.gps.seconds = 1; t.gps.milliseconds = 200;
  t.gps.fixquality = 1; t.gps.satellites = 8; t.gps.lat_hemi = 'N'; t.gps.latitude = 37.5; t.gps.lon_hemi = 'E'; t.gps.longitude = 127.25;
  t.obs.loc_source = loc::SRC_GPS; t.obs.time_source = loc::TIME_GPS;
  u.open(2);
  CHECK_STR(u.f.lines[1], "*3Status");
  CHECK_STR(u.f.lines[2] + 1, "GPS");
  CHECK_STR(u.f.lines[7] + 1, "Health");
  u.click();                                     // GPS
  CHECK_STR(u.f.lines[1], "*GPS");
  CHECK_STR(u.f.lines[2], "Date: 2026/11/21");
  CHECK_STR(u.f.lines[3], "Time:   09:05:01.200");
  CHECK_STR(u.f.lines[4], "  GPS Sat  8");
  CHECK_STR(u.f.lines[5], "Pos GPS  Time GPS");
  CHECK_STR(u.f.lines[6], " N  37.50000000 DEG");
  t.gps.link = false;                            // no NMEA for 3 s: last values + NoLink
  u.step(0);
  CHECK_STR(u.f.lines[4], "  GPS Sat  8 NoLink");
  CHECK_STR(u.f.lines[2], "Date: 2026/11/21");
  t.gps.link = true;
  u.step(0);
  CHECK_STR(u.f.lines[4], "  GPS Sat  8");
  u.step(4);                                     // info screens ignore the dial
  u.click();
  CHECK(u.ui.manager().depth() == 2);
  u.step(3); u.click();                          // IMU Calib
  u.ctx.state.cfg.accel_bias[0] = 0.125f; u.ctx.state.cfg.accel_scale[0] = 1.01f;
  u.step();
  CHECK_STR(u.f.lines[1], "*IMU Calib");
  CHECK_STR(u.f.lines[2], "Ax b +0.1250 s1.0100");
  u.click();
  CHECK(u.ui.manager().depth() == 2);
}

/************************************/
// RA Speed
/************************************/
TEST(ui_ra_speed_edit_changes_command) {
  UiFixture u;
  u.ctx.state.tracking = true;
  u.open(5);
  CHECK_STR(u.f.lines[1], "*6RA Speed");
  u.click();                                     // edit RRcmd : manual speed stops tracking
  CHECK(u.ui.raSpeed().editing());
  CHECK(!u.ctx.state.tracking);
  u.step(+5);
  CHECK_NEAR(u.ctx.state.ra_speed_cmd, 5.0, 1e-6);
  CHECK_STR(u.f.lines[1], "*Set RA Speed");
  u.step(-8);
  CHECK_NEAR(u.ctx.state.ra_speed_cmd, -3.0, 1e-6);
  CHECK(u.has(2, "-3.00000PWM"));
  CHECK_STR(u.f.lines[4], " Exit");
  u.click();                                     // finish editing
  CHECK(!u.ui.raSpeed().editing());
  u.step(+1);                                    // now moves the cursor, not the speed
  CHECK_NEAR(u.ctx.state.ra_speed_cmd, -3.0, 1e-6);
  CHECK(u.cursorAt(3) != ' ');                   // cursor on RRate
  u.click();                                     // RRate : read only
  CHECK(u.ui.manager().depth() == 2);
}

/************************************/
// Calibration (DES-CAL)
/************************************/
TEST(ui_gyro_calibration_apply_stops_motor_while_open) {
  UiFixture u;
  u.ctx.tel.imu.raw.az = 9.80665f;
  u.ctx.tel.imu.raw.wx = 0.5f; u.ctx.tel.imu.raw.wy = -0.2f; u.ctx.tel.imu.raw.wz = 0.1f;
  u.open(3);
  CHECK_STR(u.f.lines[1], "*4Calibration");
  CHECK(!u.ctx.state.motor_inhibit);
  u.click();                                     // Gyro Bias
  CHECK_STR(u.f.lines[1], "*Gyro Calib");
  CHECK_STR(u.f.lines[2], "Keep still  0/30 s");
  CHECK(u.ctx.state.motor_inhibit);              // DES-SYS-02
  u.runTo(16000);
  CHECK_STR(u.f.lines[2], "Measuring 15/30 s");
  CHECK(!u.ctx.state.settings_save_request);
  u.runTo(31100);
  CHECK_STR(u.f.lines[2], "Done. Apply?");
  CHECK(u.has(3, "+0.50000"));
  CHECK(u.has(7, "Apply") && u.has(7, "Discard"));
  CHECK(u.ctx.state.cfg.gyro_bias[0] == 0.0f);       // not used before Apply
  u.click();                                     // Apply
  CHECK(u.ctx.state.settings_save_request);
  CHECK_NEAR(u.ctx.state.cfg.gyro_bias[0], 0.5, 1e-4);
  CHECK_NEAR(u.ctx.state.cfg.gyro_bias[1], -0.2, 1e-4);
  CHECK(u.ui.manager().depth() == 2);
  CHECK(!u.ctx.state.motor_inhibit);             // the motor may run again
}

TEST(ui_gyro_calibration_shock_and_discard) {
  UiFixture u;
  u.ctx.tel.imu.raw.az = 9.80665f;
  u.open(3); u.click();
  u.runTo(5000);
  u.ctx.tel.imu.raw.wx = 20.0f;                      // someone touched the mount
  u.runTo(5100);
  CHECK_STR(u.f.lines[2], "Shock detected.");
  u.ctx.tel.imu.raw.wx = 0.0f;
  u.click();                                     // Retry
  CHECK_STR(u.f.lines[2], "Keep still  0/30 s");
  u.runTo(40000);
  CHECK_STR(u.f.lines[2], "Done. Apply?");
  u.step(+1); u.click();                         // Discard
  CHECK(!u.ctx.state.settings_save_request);
  CHECK(u.ui.manager().depth() == 2);
}

TEST(ui_accel_calibration_first_step_and_cancel) {
  UiFixture u;
  u.ctx.tel.imu.raw.az = 9.80665f;                   // Z up, but X+ is asked first
  u.open(3);
  CHECK_STR(u.f.lines[2] + 1, "Gyro Bias");
  CHECK_STR(u.f.lines[3], " Accel 6-pos");
  CHECK_STR(u.f.lines[4], " Back");
  u.step(1); u.click();
  CHECK_STR(u.f.lines[1], "*Accel Calib");
  CHECK_STR(u.f.lines[2], "Step 1/6 X+ up");
  CHECK_STR(u.f.lines[3], "Turn X+ up, hold");
  CHECK(u.ctx.state.motor_inhibit);
  u.ctx.tel.imu.raw.az = 0.0f; u.ctx.tel.imu.raw.ax = 9.80665f;
  u.runTo(500);
  CHECK_STR(u.f.lines[3], "Hold still...");
  u.click();                                     // Cancel
  CHECK(u.ui.manager().depth() == 2);
  CHECK(u.cursorAt(3) != ' ' && u.has(3, "Accel 6-pos"));   // back on the item
  CHECK(u.ctx.state.cfg.accel_scale[0] == 1.0f);
}

/************************************/
// Polar alignment (DES-PA)
/************************************/
TEST(ui_polar_align_altitude_guidance_and_confirm) {
  UiFixture u;
  known(u.ctx.tel, 37.5);
  u.ctx.tel.scope.tube_alt_deg = 36.0;
  u.ctx.tel.scope.polaris_valid = true; u.ctx.tel.scope.polaris_ha_deg = 90.0;
  u.open(1);
  CHECK_STR(u.f.lines[1], "*2Polar Align");
  CHECK(u.ctx.state.motor_inhibit);              // DES-PA-01
  // the cursor on Confirm (line 7 of 8): window starts at line 2
  CHECK_STR(u.f.lines[2], "ALT +36.00 LAT +37.50");
  CHECK_STR(u.f.lines[3], "dALT  -1.50 UP");
  CHECK_STR(u.f.lines[4], "dAZ    --- no mag");
  CHECK_STR(u.f.lines[5], "Polaris at  9:00");
  CHECK_STR(u.f.lines[6], "State: adjust");
  CHECK_STR(u.f.lines[7] + 1, "(Confirm: wait OK)");
  u.click();                                     // not OK yet: nothing
  CHECK(!u.ctx.state.polar_aligned);
  u.ctx.tel.scope.tube_alt_deg = 38.5;
  u.step();
  CHECK_STR(u.f.lines[3], "dALT  +1.00 DOWN");
  u.ctx.tel.scope.tube_alt_deg = 37.6;
  u.runTo(1000);
  CHECK_STR(u.f.lines[3], "dALT  +0.10 OK");
  CHECK_STR(u.f.lines[6], "State: adjust");      // held for 2 s first
  u.runTo(3200);
  CHECK_STR(u.f.lines[6], "ALT OK. AZ: Polaris");
  CHECK_STR(u.f.lines[7] + 1, "Confirm aligned");
  u.click();
  CHECK(u.ctx.state.polar_aligned);
  CHECK_STR(u.f.lines[6], "State: ALIGNED");
  CHECK_STR(u.f.lines[7] + 1, "Clear aligned");
  u.step(1);                                     // Back : page scrolls one line
  CHECK_STR(u.f.lines[2], "dALT  +0.10 OK");
  CHECK_STR(u.f.lines[7] + 1, "Back");
  u.click();
  CHECK(u.ui.manager().depth() == 1);
  CHECK(u.ctx.state.polar_aligned);              // kept after leaving
  CHECK(!u.ctx.state.motor_inhibit);
}

TEST(ui_polar_align_without_location) {
  UiFixture u;
  u.ctx.tel.scope.tube_alt_deg = 12.0;
  u.open(1);
  CHECK_STR(u.f.lines[2], "ALT +12.00 LAT    ---");
  CHECK_STR(u.f.lines[3], "dALT   --- no loc");
  CHECK_STR(u.f.lines[5], "Polaris       ---");
}

/************************************/
// Target : guidance, tracking (DES-PNT, DES-TRK)
/************************************/
TEST(ui_target_select_guidance_and_tracking) {
  UiFixture u;
  Telemetry& t = u.ctx.tel;
  u.open(0);
  CHECK_STR(u.f.lines[1], "*1Target");
  CHECK_STR(u.f.lines[2] + 1, "Moon");
  CHECK(u.cursorAt(3) != ' ');                   // starts on the current target (Mercury)
  u.step(2); u.click();                          // Mars
  CHECK(u.ctx.state.cfg.track_target == 3);
  CHECK(u.ctx.state.cfg.track_mode == UiState::TRACK_SIDEREAL);
  CHECK_STR(u.f.lines[1], "*Mars    computing...");
  t.target_index = 3;                            // computed, no position/time
  u.step();
  CHECK_STR(u.f.lines[1], "*Mars    no loc/time");
  // cursor on Track (line 8 of 11): window starts at T.R.A.
  CHECK_STR(u.f.lines[2], "T.R.A.       ---");
  CHECK_STR(u.f.lines[4], "dRA          ---");
  CHECK_STR(u.f.lines[6], "ALT   --- AZ   ---");
  CHECK(u.cursorAt(7) != ' ');
  CHECK(u.has(7, "Track OFF 15.0411\"/s"));     // sidereal needs no position
  u.click();                                     // Track on
  CHECK(u.ctx.state.tracking);
  // target and tube known, mount not confirmed aligned
  t.target.valid = true;
  t.target.ra_deg = 151.48; t.target.dec_deg = 13.93; t.target.alt_deg = 40.25; t.target.az_deg = 120.5;
  t.scope.valid = true; t.scope.ra_deg = 140.0; t.scope.dec_deg = 15.0;
  t.ra_calibrated = true;
  u.step();
  CHECK_STR(u.f.lines[1], "*Mars");
  CHECK_STR(u.f.lines[2], "T.R.A. 140.00000 DEG?");
  CHECK_STR(u.f.lines[3], "T.DEC.  15.00000 DEG?");
  CHECK_STR(u.f.lines[4], "dRA    +45.92 min E");
  CHECK_STR(u.f.lines[5], "dDEC    -1.07 deg S");
  CHECK_STR(u.f.lines[6], "ALT +40.25 AZ 120.50");
  CHECK(u.has(7, "Track ON  15.0411"));
  u.step(-1);                                    // up from Track: the page scrolls, no item selected
  CHECK_STR(u.f.lines[2], "DEC.    13.93000 DEG");
  CHECK_STR(u.f.lines[7], "ALT +40.25 AZ 120.50");
  u.step(-5);                                    // stops at the first line
  CHECK_STR(u.f.lines[2], "R.A.   151.48000 DEG");
  u.click();                                     // a click while scrolled up does nothing
  CHECK(u.ctx.state.tracking);
  CHECK(u.ui.manager().depth() == 3);
  u.step(+2);                                    // back on Track
  CHECK(u.cursorAt(7) != ' ');
  // tube close to the target: ON TARGET (guide tolerance 1 deg)
  t.scope.ra_deg = 151.2; t.scope.dec_deg = 13.5;
  u.ctx.state.polar_aligned = true;
  u.step();
  CHECK_STR(u.f.lines[1], "*Mars    ON TARGET");
  CHECK(!u.has(2, "?"));
  CHECK_STR(u.f.lines[4], "dRA     +1.12 min E");
  CHECK_STR(u.f.lines[5], "dDEC    +0.43 deg N");
  u.ctx.state.cfg.guide_tolerance_deg = 0.2f;
  u.step();
  CHECK_STR(u.f.lines[1], "*Mars");
  // Rate : Lunar needs the Moon
  u.step(1); u.click();
  CHECK(u.ctx.state.cfg.track_mode == UiState::TRACK_LUNAR);
  CHECK_STR(u.f.lines[7] + 1, "Rate Lunar");
  CHECK_STR(u.f.lines[6], " Track ON    ---");
  t.moon.valid = true; t.moon.rate_arcsec_s = 14.4856;
  u.step();
  CHECK_STR(u.f.lines[6], " Track ON  14.4856\"/s");
  u.click();
  CHECK(u.ctx.state.cfg.track_mode == UiState::TRACK_SIDEREAL);
  // Offset : edit with the dial
  u.step(1); u.click();
  u.step(3);
  CHECK_NEAR(u.ctx.state.cfg.track_offset, 3.0, 1e-6);
  u.step(-5);
  CHECK_NEAR(u.ctx.state.cfg.track_offset, -2.0, 1e-6);
  CHECK_STR(u.f.lines[7] + 1, "Offset     -2.0 PWM");
  u.click();                                     // done; the dial moves the cursor again
  u.step(1);
  CHECK_NEAR(u.ctx.state.cfg.track_offset, -2.0, 1e-6);
  CHECK_STR(u.f.lines[7] + 1, "Back");
  t.ra_calibrated = false;
  u.step();
  CHECK_STR(u.f.lines[7] + 1, "Back   (RA uncal)");
  u.click();                                     // Back : tracking stays on
  CHECK(u.ctx.state.tracking);
  CHECK_STR(u.f.lines[1], "*1Target");
  CHECK_STR(u.f.lines[5] + 1, "Mars     TRACKING");
  u.step(-3); u.click();                         // Moon : starts stopped, lunar rate
  CHECK(u.ctx.state.cfg.track_target == 0);
  CHECK(!u.ctx.state.tracking);
  CHECK(u.ctx.state.cfg.track_mode == UiState::TRACK_LUNAR);
  u.step(0);
  u.click();                                     // same target again keeps the settings
  CHECK(u.ctx.state.tracking);
  u.step(3); u.click();                          // Back to the target menu
  u.click();
  CHECK(u.ctx.state.cfg.track_target == 0);
  CHECK(u.ctx.state.tracking);
  CHECK(u.ctx.state.cfg.track_mode == UiState::TRACK_LUNAR);
}

/************************************/
// Settings, Location, Time (DES-CFG, DES-LOC)
/************************************/
TEST(ui_settings_items_and_scrolling) {
  UiFixture u;
  u.ctx.tel.settings_status = Telemetry::SETTINGS_LOADED;
  u.ctx.state.ra_speed_cmd = 123.0f;             // found on RA Speed
  u.open(4);
  CHECK_STR(u.f.lines[1], "*5Settings");
  CHECK_STR(u.f.lines[2], "Flash: loaded");
  CHECK_STR(u.f.lines[3], "RAduty      +0.0 PWM");
  CHECK_STR(u.f.lines[4] + 1, "RAduty = RRcmd");
  CHECK(u.cursorAt(4) != ' ');
  CHECK_STR(u.f.lines[5] + 1, "Location >");
  CHECK_STR(u.f.lines[6] + 1, "Time >");
  CHECK_STR(u.f.lines[7] + 1, "Motor dir Normal");
  u.click();                                     // RAduty = RRcmd
  CHECK(u.ctx.state.cfg.ra_duty_at_sidereal == 123.0f);
  CHECK(u.ctx.state.settings_save_request);
  CHECK_STR(u.f.lines[3], "RAduty    +123.0 PWM");
  u.ctx.state.settings_save_request = false;
  u.step(3); u.click();                          // Motor dir
  CHECK(u.ctx.state.cfg.motor_reverse);
  CHECK_STR(u.f.lines[7] + 1, "Motor dir Reverse");
  u.step(1);                                     // Guide tol : page scrolls
  CHECK_STR(u.f.lines[2], "RAduty    +123.0 PWM");
  CHECK_STR(u.f.lines[7] + 1, "Guide tol  1.00 deg");
  u.click(); u.step(5); u.click();
  CHECK_NEAR(u.ctx.state.cfg.guide_tolerance_deg, 1.5, 1e-5);
  u.step(-30);                                   // not editing: the cursor goes to the top
  CHECK(u.cursorAt(4) != ' ');
  u.step(4); u.click(); u.step(-30); u.click();  // tolerance clamps at 0.1
  CHECK_NEAR(u.ctx.state.cfg.guide_tolerance_deg, 0.1, 1e-5);
  u.step(1); u.click();                          // Save
  CHECK(u.ctx.state.settings_save_request);
}

TEST(ui_settings_reset_needs_a_second_click) {
  UiFixture u;
  u.open(4);
  u.step(6); u.click();                          // Reset defaults : first click
  CHECK_STR(u.f.lines[7] + 1, "Reset? click again");
  CHECK(!u.ctx.state.settings_reset_request);
  u.step(1); u.step(-1);                         // turning cancels
  CHECK_STR(u.f.lines[7] + 1, "Reset defaults");
  u.click(); u.click();
  CHECK(u.ctx.state.settings_reset_request);
  u.step(1); u.click();                          // Back
  CHECK(u.ui.manager().depth() == 1);
}

TEST(ui_location_manual_entry) {
  UiFixture u;
  Telemetry& t = u.ctx.tel;
  u.open(4); u.step(1); u.click();               // Location >
  CHECK_STR(u.f.lines[1], "*Location");
  CHECK_STR(u.f.lines[2], "In use: ---");
  CHECK_STR(u.f.lines[3] + 1, "Mode Auto");
  CHECK_STR(u.f.lines[4], " Lat   +0.0000 deg");
  u.click();                                     // Mode
  CHECK(u.ctx.state.cfg.location_mode == loc::MODE_MANUAL);
  CHECK_STR(u.f.lines[3] + 1, "Mode Manual");
  u.step(1); u.click();                          // edit Lat, tens first
  CHECK_STR(u.f.lines[1], "*Lat step 10");
  CHECK(u.cursorAt(4) == '*');
  u.step(3);
  CHECK(u.ctx.state.cfg.manual_set);
  u.click(); u.step(7);                          // ones
  u.click(); u.step(5);                          // tenths
  CHECK_STR(u.f.lines[4], "*Lat  +37.5000 deg");
  u.step(+20);                                   // clamped at 90
  CHECK_NEAR(u.ctx.state.cfg.manual_lat_deg, 39.5, 1e-9);
  for (int i = 0; i < 4; i++) u.click();         // through the last digit: done
  CHECK_STR(u.f.lines[1], "*Location");
  CHECK(u.cursorAt(4) != '*');
  u.step(3);                                     // Copy GPS fix: none
  CHECK_STR(u.f.lines[7] + 1, "Copy GPS fix (none)");
  u.click();
  CHECK_NEAR(u.ctx.state.cfg.manual_lat_deg, 39.5, 1e-9);
  t.gps.fixquality = 1; t.gps.latitude = 35.1796; t.gps.longitude = 129.0756; t.gps.altitude_m = 12.0f;
  u.click();
  CHECK_NEAR(u.ctx.state.cfg.manual_lat_deg, 35.1796, 1e-9);
  CHECK_NEAR(u.ctx.state.cfg.manual_lon_deg, 129.0756, 1e-9);
  t.obs.loc_source = loc::SRC_MANUAL;
  u.step(1); u.click();                          // Save
  CHECK(u.ctx.state.settings_save_request);
  u.step(-10);
  CHECK_STR(u.f.lines[2], "In use: MAN");
  u.step(10); u.click();                         // Back to Settings
  CHECK(u.ui.manager().depth() == 2);
  CHECK_STR(u.f.lines[1], "*5Settings");
}

TEST(ui_time_manual_clock) {
  UiFixture u;
  u.ctx.tel.now_ms = 5000;
  u.open(4); u.step(2); u.click();               // Time >
  CHECK_STR(u.f.lines[1], "*Time (local)");
  CHECK_STR(u.f.lines[2], "In use: ---");
  CHECK_STR(u.f.lines[3] + 1, "Year   2026");
  u.step(1); u.click(); u.step(10); u.click();   // Month 1 + 10 = 11
  u.step(1); u.click(); u.step(-7); u.click();   // Day 1 - 7 wraps to 25
  u.step(1); u.click(); u.step(-3); u.click();   // Hour 0 - 3 wraps to 21
  CHECK_STR(u.f.lines[4], " Month  11");
  CHECK_STR(u.f.lines[5], " Day    25");
  CHECK_STR(u.f.lines[6] + 1, "Hour   21");
  u.step(3); u.click(); u.step(36); u.click();   // UTC +09:00
  CHECK(u.ctx.state.cfg.utc_offset_min == 540);
  CHECK_STR(u.f.lines[7] + 1, "UTC +09:00");
  u.step(1); u.click();                          // Set clock
  CHECK(u.ctx.state.manual_clock_valid);
  CHECK(u.ctx.state.manual_clock_ms == 5000);
  CHECK(u.ctx.state.settings_save_request);
  CHECK_NEAR(u.ctx.state.manual_clock_jd, astro::julianDate({2026, 11, 25, 12, 0, 0.0}), 1e-9);
  CHECK_STR(u.f.lines[7] + 1, "Set clock  (set)");
  // the loop now uses it; the page shows it in local time
  u.ctx.tel.obs.time_source = loc::TIME_MANUAL;
  u.ctx.tel.obs.jd_ut = u.ctx.state.manual_clock_jd;
  u.step(-10);
  CHECK_STR(u.f.lines[2], "In use MAN 21:00:00");
  u.step(10); u.click();
  CHECK_STR(u.f.lines[1], "*5Settings");
}

TEST(ui_time_set_clamps_the_day) {
  UiFixture u;
  u.open(4); u.step(2); u.click();
  u.step(1); u.click(); u.step(1); u.click();    // February
  u.step(1); u.click(); u.step(-1); u.click();   // Day 31
  u.step(5); u.click();                          // Set clock
  CHECK_STR(u.f.lines[7] + 1, "Set clock  (set)");
  CHECK_NEAR(u.ctx.state.manual_clock_jd, astro::julianDate({2026, 2, 28, 0, 0, 0.0}), 1e-9);
}

/************************************/
// Every screen with large values: no line overflows (UiFixture::step checks it)
/************************************/
TEST(ui_lines_never_overflow) {
  UiFixture u;
  Telemetry& t = u.ctx.tel;
  UiState& s = u.ctx.state;
  known(t, -89.9999);
  t.obs.jd_ut = astro::julianDate({2026, 11, 24, 23, 59, 59.9});
  t.gps.fixquality = 2; t.gps.satellites = 24; t.gps.latitude = -89.99999999; t.gps.longitude = -179.99999999;
  t.imu.raw.ax = -156.9f; t.imu.raw.wx = -999.0f; t.imu.raw.mx = -4800.0f; t.imu.raw.temperature = -40.0f;
  t.imu.corr = t.imu.raw;
  t.imu.roll_acc_deg = -180.0f; t.imu.pitch_acc_deg = -90.0f; t.imu.roll_est_deg = -180.0f; t.imu.pitch_est_deg = -90.0f;
  t.health.loop_us = 999999; t.health.loop_hz = 999.9f; t.health.main_us = 999999; t.health.est_us = 999999; t.health.astro_us = 99999;
  t.scope.tube_alt_deg = -90.0; t.scope.polaris_valid = true; t.scope.polaris_ha_deg = -179.99;
  t.target.valid = true; t.target_index = 1; t.target.ra_deg = 359.99999; t.target.dec_deg = -89.9;
  t.target.alt_deg = -90.0; t.target.az_deg = 359.99;
  t.scope.valid = true; t.scope.ra_deg = 0.00001; t.scope.dec_deg = 89.9;
  s.ra_speed_cmd = -9999.0f; s.cfg.ra_duty_at_sidereal = -9999.0f; s.cfg.track_offset = -9999.0f;
  s.cfg.gyro_bias[0] = -999.0f; s.cfg.accel_bias[0] = -99.0f; s.cfg.accel_scale[0] = 1.1f;
  s.cfg.manual_lat_deg = -90.0; s.cfg.manual_lon_deg = -180.0; s.cfg.manual_alt_m = -1000.0f;
  s.cfg.utc_offset_min = -720; s.cfg.guide_tolerance_deg = 20.0f;
  // sub pages opened from each main item (-1 = none)
  static const int SUBS[SimpleGoToUi::MAIN_ITEMS][7] = {
    {1, -1},                          // Target menu : Mercury
    {-1}, {0, 1, 2, 3, 4, 5, -1},     // Polar Align ; Status screens
    {0, 1, -1},                       // Calibration : Gyro, Accel
    {1, 2, -1},                       // Settings : Location, Time
    {-1}, {-1},
  };
  for (int item = 0; item < SimpleGoToUi::MAIN_ITEMS; item++) {
    u.step(-20); u.step(item); u.click();
    for (int k = 0; k < 12; k++) u.step(1);      // every line of the page
    for (int j = 0; SUBS[item][j] >= 0; j++) {
      u.step(-20); u.step(SUBS[item][j]); u.click();
      CHECK(u.ui.manager().depth() == 3);
      for (int k = 0; k < 12; k++) u.step(1);
      for (int k = 0; k < 12; k++) u.step(-1);   // scrolled up to the first line too
      if (item == 3) u.runTo(t.now_ms + 32000);  // calibration result pages
      while (u.ui.manager().depth() > 2) { u.step(20); u.click(); }
    }
    while (u.ui.manager().depth() > 1) { u.step(20); u.click(); }
    CHECK(u.ui.manager().depth() == 1);
  }
}
