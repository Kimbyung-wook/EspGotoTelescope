#include "telescope_ui.h"

static const char* const MAIN_MENU_LABELS[MainMenuScreen::ITEMS] = {
  "1RA Speed",
  "2Status - GPS",
  "3Status - Acc/Gyro",
  "4Status - Mag/Alt",
  "5Status - IMUCalib",
  "6Do IMU Calib.",
  "7Att.Estimator",
  "8Health",
  "9Axis for Moon",
  "Sleep",
};

TelescopeUi::TelescopeUi()
  : ra_speed_(MAIN_MENU_LABELS[0]),
    gps_(MAIN_MENU_LABELS[1]),
    acc_gyro_(MAIN_MENU_LABELS[2]),
    mag_temp_(MAIN_MENU_LABELS[3]),
    imu_calib_status_(MAIN_MENU_LABELS[4]),
    imu_calib_menu_(MAIN_MENU_LABELS[5]),
    att_est_(MAIN_MENU_LABELS[6]),
    health_(MAIN_MENU_LABELS[7]),
    moon_axis_(MAIN_MENU_LABELS[8]),
    targets_{&ra_speed_, &gps_, &acc_gyro_, &mag_temp_, &imu_calib_status_,
             &imu_calib_menu_, &att_est_, &health_, &moon_axis_, &sleep_},
    main_(MAIN_MENU_LABELS, targets_),
    mgr_(&main_) {}

void TelescopeUi::step(int rotate_delta, bool click, UiContext& ctx, Frame& f) {
  const Telemetry& t = ctx.tel;
  f.set(0, "T %2d:%2d:%2d.%1d V %5.2f",
        t.hour % 100, t.minute % 100, t.seconds % 100, (t.milliseconds / 100) % 10, t.voltage);
  mgr_.step(rotate_delta, click, ctx, f);
}
