#pragma once
#ifndef _UI_SCREENS_SETTINGS_H_
#define _UI_SCREENS_SETTINGS_H_
#include "widgets.h"

/************************************/
// Settings (DES-CFG-05) and its Location / Time pages (DES-LOC-01..04)
//   the main loop does the flash work : UiState::settings_*_request
/************************************/
// Location : mode, manual position (click a value, turn, click for the next digit)
class LocationScreen : public PageScreen {
public:
  enum Item { MODE = 0, LAT, LON, ALT, FROM_GPS, SAVE, BACK, ITEMS };
  static const int INFO_LINES = 1;
  LocationScreen() : PageScreen(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
  double step() const;                          // current digit of the edited value
private:
  int stepCount() const;
  bool editing_ = false;
  int step_ = 0;
};

// Time : manual date/time (local), UTC offset, Set clock
class TimeScreen : public PageScreen {
public:
  enum Item { YEAR = 0, MONTH, DAY, HOUR, MINUTE, SECOND, UTC_OFFSET, SET, BACK, ITEMS };
  static const int INFO_LINES = 1;
  static const int OFFSET_STEP_MIN = 15;
  TimeScreen() : PageScreen(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& ctx) override;
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
private:
  bool editing_ = false;
  bool set_ = false;                             // "Set clock" done since entering
  int v_[6] = {2026, 1, 1, 0, 0, 0};             // local year, month, day, hour, minute, second
};

class SettingsScreen : public PageScreen {
public:
  enum Item { SET_RA_DUTY = 0, LOCATION, TIME, MOTOR_DIR, GUIDE_TOL, SAVE, RESET, BACK, ITEMS };
  static const int INFO_LINES = 2;               // Flash status, RAduty
  static constexpr float TOL_STEP = 0.1f;
  explicit SettingsScreen(const char* title) : PageScreen(INFO_LINES, ITEMS), title_(title) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); confirm_reset_ = false; editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
private:
  const char* title_;
  bool confirm_reset_ = false;   // Reset needs a second click
  bool editing_ = false;         // Guide tol
  LocationScreen location_;
  TimeScreen time_;
};

#endif // _UI_SCREENS_SETTINGS_H_
