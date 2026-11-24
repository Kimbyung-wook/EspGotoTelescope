#pragma once
#ifndef _TELESCOPE_UI_H_
#define _TELESCOPE_UI_H_
#include "screens.h"

/************************************/
// The whole OLED UI. One call per frame :
//   ui.step(rotary_delta, clicked, ctx, frame);
// Hardware independent : also used by host unit tests and the PC simulator.
//
// Menu (docs/REQUIREMENTS.md DES-UI-01):
//   1Target  2Polar Align  3Status >  4Calibration >  5Settings  6RA Speed  Sleep
/************************************/
class TelescopeUi {
public:
  static const int MAIN_ITEMS = 7;
  static const int STATUS_ITEMS = 7;     // 6 screens + Back
  static const int CALIB_ITEMS = 3;      // Gyro, Accel, Back

  TelescopeUi();
  void step(int rotate_delta, bool click, UiContext& ctx, Frame& f);
  ScreenManager& manager() { return mgr_; }
  MenuScreen& mainMenu() { return main_; }
  RaSpeedScreen& raSpeed() { return ra_speed_; }

private:
  // Target, Polar Align
  TargetMenuScreen     target_menu_;
  PolarAlignScreen     polar_align_;
  // Status
  GpsStatusScreen      gps_;
  AccGyroScreen        acc_gyro_;
  MagTempScreen        mag_temp_;
  ImuCalibStatusScreen imu_calib_status_;
  AttEstScreen         att_est_;
  HealthScreen         health_;
  Screen* status_targets_[STATUS_ITEMS];
  MenuScreen           status_;
  // Calibration
  GyroCalibScreen      gyro_calib_;
  AccelCalibScreen     accel_calib_;
  Screen* calib_targets_[CALIB_ITEMS];
  MenuScreen           calib_;
  // Settings, RA Speed, Sleep
  SettingsScreen       settings_;
  RaSpeedScreen        ra_speed_;
  SleepScreen          sleep_;
  Screen* main_targets_[MAIN_ITEMS];
  MenuScreen     main_;
  ScreenManager  mgr_;
};

#endif // _TELESCOPE_UI_H_
