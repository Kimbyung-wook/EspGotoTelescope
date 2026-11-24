#include "test_main.h"
#include "../app/src/app/loop_logic.h"
#include "../app/src/sensors/battery.h"

using namespace loc;

namespace {
Position at(double lat, double lon, float alt = 0.0f) {
  Position p; p.lat_deg = lat; p.lon_deg = lon; p.alt_m = alt;
  return p;
}
void gpsFix(Telemetry& t, int fix = 1, int sats = 8) {
  t.gps.year = 26; t.gps.month = 11; t.gps.day = 24; t.gps.hour = 12; t.gps.minute = 30; t.gps.seconds = 15; t.gps.milliseconds = 500;
  t.gps.fixquality = fix; t.gps.satellites = sats;
  t.gps.latitude = 37.5665; t.gps.longitude = 126.9780; t.gps.altitude_m = 38.0f;
}
}

TEST(loc_resolve_order_auto_and_manual) {
  const Position gps = at(1, 2), last = at(3, 4), man = at(5, 6);
  Position out;
  CHECK(resolvePosition(MODE_AUTO, true, gps, true, last, true, man, out) == SRC_GPS);
  CHECK(out.lat_deg == 1);
  CHECK(resolvePosition(MODE_AUTO, false, gps, true, last, true, man, out) == SRC_LAST);
  CHECK(out.lat_deg == 3);
  CHECK(resolvePosition(MODE_AUTO, false, gps, false, last, true, man, out) == SRC_MANUAL);
  CHECK(out.lat_deg == 5);
  CHECK(resolvePosition(MODE_AUTO, false, gps, false, last, false, man, out) == SRC_NONE);
  CHECK(resolvePosition(MODE_MANUAL, true, gps, true, last, true, man, out) == SRC_MANUAL);
  CHECK(out.lat_deg == 5);
  CHECK(resolvePosition(MODE_MANUAL, true, gps, true, last, false, man, out) == SRC_NONE);
}

TEST(loc_distance_km) {
  CHECK_NEAR(distanceKm(at(0, 0), at(0, 1)), 111.19, 0.01);       // one degree on the equator
  CHECK_NEAR(distanceKm(at(37.5665, 126.978), at(37.5665, 126.978)), 0.0, 1e-9);
  CHECK(distanceKm(at(37.5665, 126.978), at(37.5755, 126.978)) < 1.01);   // ~1.0 km north
}

TEST(loc_last_gps_needs_30_s_and_4_satellites) {
  LastGpsRecorder r;
  const Position p = at(37.5665, 126.978);
  CHECK(!r.update(0, 1, 3, p, false, p));          // 3 satellites: not good enough
  CHECK(!r.update(100, 1, 8, p, false, p));        // good fix starts the 30 s
  CHECK(!r.update(20000, 1, 8, p, false, p));
  CHECK(!r.update(25000, 0, 0, p, false, p));      // lost: starts over
  CHECK(!r.update(26000, 1, 8, p, false, p));
  CHECK(!r.update(55000, 1, 8, p, false, p));
  CHECK(r.update(56000, 1, 8, p, false, p));       // held 30 s, nothing saved yet -> save
  CHECK(r.done());
  CHECK(!r.update(90000, 1, 8, p, false, p));      // once per power-on
}

TEST(loc_last_gps_only_when_moved_1_km) {
  const Position p = at(37.5665, 126.978);
  LastGpsRecorder same;
  same.update(0, 1, 8, p, false, p);
  CHECK(!same.update(30000, 1, 8, p, true, at(37.5670, 126.978)));   // 55 m away: keep
  CHECK(same.done());
  LastGpsRecorder moved;
  moved.update(0, 1, 8, p, false, p);
  CHECK(moved.update(30000, 1, 8, p, true, at(35.1796, 129.0756)));  // Busan
}

TEST(loc_update_uses_gps_and_records_the_fix) {
  Telemetry t; UiState st; LastGpsRecorder rec;
  gpsFix(t);
  t.now_ms = 1000;
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.loc_source == SRC_GPS && t.obs.time_source == TIME_GPS);
  CHECK_NEAR(t.obs.lat_deg, 37.5665, 1e-12);
  CHECK_NEAR(t.obs.jd_ut, astro::julianDate({2026, 11, 24, 12, 30, 15.5}), 1e-9);
  CHECK(t.obs.clock_hour == 12 && t.obs.clock_minute == 30 && t.obs.clock_second == 15 && t.obs.clock_ms == 500);
  CHECK(!st.cfg.last_gps_valid);
  t.now_ms = 31000;
  app::updateLocationTime(t, st, rec);
  CHECK(st.cfg.last_gps_valid);                         // kept after 30 s of fix
  CHECK(st.settings_save_request);
  CHECK(st.cfg.last_year == 2026 && st.cfg.last_month == 11 && st.cfg.last_day == 24);
  CHECK_NEAR(st.cfg.last_lat_deg, 37.5665, 1e-12);
  // GPS lost: the last fix and no time
  t.gps.fixquality = 0;
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.loc_source == SRC_LAST);
  CHECK(t.obs.time_source == TIME_NONE);
}

// GPS unplugged: the fields keep the last fix, but it is not used any more
TEST(loc_link_lost_drops_the_stale_gps_fix) {
  Telemetry t; UiState st; LastGpsRecorder rec;
  st.cfg.manual_set = true; st.cfg.manual_lat_deg = -33.8688; st.cfg.manual_lon_deg = 151.2093;
  gpsFix(t);
  t.gps.link = false;
  for (unsigned long ms = 0; ms <= 40000; ms += 1000) {   // longer than the 30 s of DES-LOC-02
    t.now_ms = ms;
    app::updateLocationTime(t, st, rec);
  }
  CHECK(t.obs.loc_source == SRC_MANUAL);
  CHECK(t.obs.time_source == TIME_NONE);
  CHECK(!st.cfg.last_gps_valid);                        // a stale fix is not recorded
  CHECK(!gpsFixUsable(t.gps));
  t.gps.link = true;                                    // back
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.loc_source == SRC_GPS && t.obs.time_source == TIME_GPS);
}

TEST(loc_manual_clock_runs_with_millis) {
  Telemetry t; UiState st; LastGpsRecorder rec;
  st.cfg.manual_set = true; st.cfg.manual_lat_deg = -33.8688; st.cfg.manual_lon_deg = 151.2093;
  st.manual_clock_valid = true;
  st.manual_clock_jd = astro::julianDate({2026, 11, 24, 23, 59, 59.0});
  st.manual_clock_ms = 5000;
  t.now_ms = 7500;                                  // 2.5 s later
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.loc_source == SRC_MANUAL && t.obs.time_source == TIME_MANUAL);
  CHECK(t.obs.clock_hour == 0 && t.obs.clock_minute == 0 && t.obs.clock_second == 1);
  CHECK(t.obs.clock_ms >= 499 && t.obs.clock_ms <= 501);
  gpsFix(t);                                        // a GPS fix wins (Auto)
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.loc_source == SRC_GPS && t.obs.time_source == TIME_GPS);
  st.cfg.location_mode = MODE_MANUAL;                   // Manual: GPS position ignored
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.loc_source == SRC_MANUAL);
  CHECK_NEAR(t.obs.lat_deg, -33.8688, 1e-12);
}

TEST(loc_time_order_gps_phone_manual) {
  Telemetry t; UiState st; LastGpsRecorder rec;
  st.manual_clock_valid = true;                      // only a clock set by hand
  st.manual_clock_jd = astro::julianDate({2026, 11, 25, 9, 0, 0.0});
  st.manual_clock_ms = 0;
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.time_source == TIME_MANUAL && t.obs.clock_hour == 9);
  app::setPhoneClock(st, 1000, 1500000000UL, 0);     // 2017: phone clock not set, ignored
  CHECK(!st.phone_clock_valid);
  app::setPhoneClock(st, 1000, 1795575845UL, 600);   // 2026-11-25 03:04:05.600 UTC
  CHECK(st.phone_clock_valid);
  CHECK_NEAR(st.phone_clock_jd, astro::julianDate({2026, 11, 25, 3, 4, 5.6}), 1e-8);
  t.now_ms = 3000;                                   // 2 s later
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.time_source == TIME_PHONE);                // the phone wins over the hand-set clock
  CHECK(t.obs.clock_hour == 3 && t.obs.clock_minute == 4 && t.obs.clock_second == 7);
  CHECK(t.obs.clock_ms >= 599 && t.obs.clock_ms <= 601);
  CHECK(std::strcmp(timeSourceText(t.obs.time_source), "PHN") == 0);
  gpsFix(t);                                         // and GPS wins over both
  app::updateLocationTime(t, st, rec);
  CHECK(t.obs.time_source == TIME_GPS);
}

TEST(battery_scale_and_filter) {
  BatteryMonitor b(15.2f, 0.5f);
  CHECK(!b.valid());
  b.update(1023);
  CHECK(b.valid());
  CHECK_NEAR(b.volts(), 15.2, 1e-4);                 // first sample: no filtering
  b.update(0);
  CHECK_NEAR(b.volts(), 7.6, 1e-4);                  // half way with alpha 0.5
  b.update(2000);                                    // out of range: clamped to 1023
  CHECK_NEAR(b.volts(), 11.4, 1e-4);
  BatteryMonitor c(15.2f, 0.2f);
  c.update(846);                                     // 12.57 V
  for (int i = 0; i < 50; i++) c.update(i & 1 ? 850 : 842);   // ADC noise +-4
  CHECK_NEAR(c.volts(), 846 * 15.2 / 1023.0, 0.02);
}

TEST(loc_jd_calendar_round_trip) {
  const astro::UtcTime cases[] = {{2026, 11, 24, 12, 30, 15.25}, {2000, 1, 1, 0, 0, 0.0},
                                  {2024, 2, 29, 23, 59, 59.5}, {2099, 12, 31, 6, 7, 8.0}};
  for (const astro::UtcTime& u : cases) {
    astro::UtcTime back;
    CHECK(astro::utcFromJd(astro::julianDate(u), back));
    CHECK(back.year == u.year && back.month == u.month && back.day == u.day);
    CHECK(back.hour == u.hour && back.minute == u.minute);
    CHECK_NEAR(back.second, u.second, 1e-3);
  }
}

TEST(loc_imu_correction_applies_gyro_and_accel) {
  Telemetry t; UiState st;
  t.imu.raw.ax = 0.1f; t.imu.raw.ay = 0.0f; t.imu.raw.az = 10.0f; t.imu.raw.wx = 0.5f;
  st.cfg.gyro_bias[0] = 0.5f;
  st.cfg.accel_bias[0] = 0.1f; st.cfg.accel_bias[2] = 0.2f; st.cfg.accel_scale[2] = 1.0f;
  app::correctImu(t, st);
  CHECK_NEAR(t.imu.corr.wx, 0.0, 1e-6);
  CHECK_NEAR(t.imu.corr.ax, 0.0, 1e-6);
  CHECK_NEAR(t.imu.corr.az, 9.8, 1e-5);
  CHECK_NEAR(t.imu.pitch_acc_deg, 0.0, 1e-4);           // tilt from the corrected sample
  CHECK_NEAR(t.imu.raw.ax, 0.1, 1e-6);                  // raw sample unchanged
}

TEST(loc_motor_inhibit_and_tracking_sign) {
  Telemetry t; UiState st;
  st.ra_speed_cmd = 40.0f;
  CHECK(app::motorCommand(t, st) == 40.0f);
  st.motor_inhibit = true;
  CHECK(app::motorCommand(t, st) == 0.0f);          // stopped, command kept
  CHECK(st.ra_speed_cmd == 40.0f);
  st.motor_inhibit = false;
  st.tracking = true; st.cfg.ra_duty_at_sidereal = 100.0f;   // sidereal: no position needed
  CHECK_NEAR(app::motorCommand(t, st), 100.0, 1e-4);
  t.obs.loc_source = SRC_MANUAL; t.obs.lat_deg = -33.0;      // southern hemisphere
  CHECK_NEAR(app::motorCommand(t, st), -100.0, 1e-4);
  st.cfg.motor_reverse = true;                                // wiring flipped as well
  CHECK_NEAR(app::motorCommand(t, st), 100.0, 1e-4);
  st.cfg.track_mode = UiState::TRACK_LUNAR;                   // no Moon yet: command kept
  st.ra_speed_cmd = 7.0f;
  CHECK_NEAR(app::motorCommand(t, st), 7.0, 1e-6);
  t.moon.valid = true; t.moon.rate_arcsec_s = 14.5;
  t.obs.lat_deg = 37.0; st.cfg.motor_reverse = false;
  CHECK_NEAR(app::motorCommand(t, st), 100.0 * 14.5 / astro::SIDEREAL_RATE_ARCSEC_S, 1e-4);
}
