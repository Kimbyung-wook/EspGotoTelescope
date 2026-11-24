#pragma once
#ifndef _TELESCOPE_UI_H_
#define _TELESCOPE_UI_H_
#include "screens.h"

/************************************/
// The whole OLED UI. One call per frame :
//   ui.step(rotary_delta, clicked, ctx, frame);
// Hardware independent : also used by host unit tests and the PC simulator.
/************************************/
class TelescopeUi {
public:
  TelescopeUi();
  void step(int rotate_delta, bool click, UiContext& ctx, Frame& f);
  ScreenManager& manager() { return mgr_; }
  MainMenuScreen& mainMenu() { return main_; }
  RaSpeedScreen& raSpeed() { return ra_speed_; }

private:
  RaSpeedScreen        ra_speed_;
  GpsStatusScreen      gps_;
  AccGyroScreen        acc_gyro_;
  MagTempScreen        mag_temp_;
  ImuCalibStatusScreen imu_calib_status_;
  ImuCalibMenuScreen   imu_calib_menu_;
  AttEstScreen         att_est_;
  HealthScreen         health_;
  MoonAxisScreen       moon_axis_;
  TargetMenuScreen     target_menu_;
  SettingsScreen       settings_;
  SleepScreen          sleep_;
  Screen* targets_[MainMenuScreen::ITEMS];
  MainMenuScreen main_;
  ScreenManager  mgr_;
};

#endif // _TELESCOPE_UI_H_
