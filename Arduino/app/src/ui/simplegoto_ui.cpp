#include "simplegoto_ui.h"
#include "../location/location.h"

static const char* const MAIN_LABELS[SimpleGoToUi::MAIN_ITEMS] = {
  "1Target",
  "2Polar Align",
  "3Status",
  "4Calibration",
  "5Settings",
  "6RA Speed",
  "Sleep",
};

static const char* const STATUS_LABELS[SimpleGoToUi::STATUS_ITEMS] = {
  "GPS", "Acc/Gyro", "Mag/Temp", "IMU Calib", "Att.Estimator", "Health", "Back",
};

static const char* const CALIB_LABELS[SimpleGoToUi::CALIB_ITEMS] = {
  "Gyro Bias", "Accel 6-pos", "Back",
};

SimpleGoToUi::SimpleGoToUi()
  : target_menu_(MAIN_LABELS[0]),
    polar_align_(MAIN_LABELS[1]),
    gps_(STATUS_LABELS[0]),
    acc_gyro_(STATUS_LABELS[1]),
    mag_temp_(STATUS_LABELS[2]),
    imu_calib_status_(STATUS_LABELS[3]),
    att_est_(STATUS_LABELS[4]),
    health_(STATUS_LABELS[5]),
    status_targets_{&gps_, &acc_gyro_, &mag_temp_, &imu_calib_status_, &att_est_, &health_, nullptr},
    status_(MAIN_LABELS[2], STATUS_LABELS, status_targets_, STATUS_ITEMS),
    calib_targets_{&gyro_calib_, &accel_calib_, nullptr},
    calib_(MAIN_LABELS[3], CALIB_LABELS, calib_targets_, CALIB_ITEMS),
    settings_(MAIN_LABELS[4]),
    ra_speed_(MAIN_LABELS[5]),
    main_targets_{&target_menu_, &polar_align_, &status_, &calib_, &settings_, &ra_speed_, &sleep_},
    main_("Main Menu", MAIN_LABELS, main_targets_, MAIN_ITEMS, true),
    mgr_(&main_) {}

void SimpleGoToUi::step(int rotate_delta, bool click, UiContext& ctx, Frame& f) {
  const Telemetry& t = ctx.tel;
  // 'T' GPS time, 'M' manual clock (DES-LOC-04), 'P' phone clock (DES-LOC-05)
  const char src = t.time_source == loc::TIME_MANUAL ? 'M' : t.time_source == loc::TIME_PHONE ? 'P' : 'T';
  char volts[8] = "--.--";
  if (t.voltage_valid) snprintf(volts, sizeof(volts), "%5.2f", t.voltage < 0.0f ? 0.0f : t.voltage > 99.99f ? 99.99f : t.voltage);
  f.set(0, "%c %2d:%2d:%2d.%1d V %s", src,
        t.clock_hour % 100, t.clock_minute % 100, t.clock_second % 100, (t.clock_ms / 100) % 10, volts);
  // Wi-Fi mark in column 20 (the time/voltage text above is 20 characters)
  if (t.remote != Telemetry::REMOTE_OFF && strlen(f.lines[0]) == Frame::COLS - 2)
    f.lines[0][Frame::COLS - 2] = t.remote == Telemetry::REMOTE_CONNECTED ? 'W' : 'w';
  mgr_.step(rotate_delta, click, ctx, f);
}
