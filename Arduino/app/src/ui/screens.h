#pragma once
#ifndef _UI_SCREENS_H_
#define _UI_SCREENS_H_
#include "screen_manager.h"
#include "../estimation/calibration.h"

/************************************/
// Screens of docs/UI.md (menu of docs/REQUIREMENTS.md DES-UI-01)
/************************************/

// List of items, each opening a screen; a null target is "Back" (pop).
// The root menu has no '*' in the title.
class MenuScreen : public Screen {
public:
  MenuScreen(const char* title, const char* const* labels, Screen* const* targets, int count,
             bool root = false)
    : title_(title), labels_(labels), targets_(targets), cursor_(count), root_(root) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); }
  void onRotate(int delta, UiContext& /*ctx*/) override { cursor_.move(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  int selected() const { return cursor_.selected(); }
  int count() const { return cursor_.count(); }
private:
  const char* title_;
  const char* const* labels_;
  Screen* const* targets_;
  MenuCursor cursor_;
  bool root_;
};

// Info screen : no cursor, click = back
class InfoScreen : public Screen {
public:
  explicit InfoScreen(const char* title) : title_(title) {}
  void onClick(ScreenManager& mgr, UiContext& /*ctx*/) override { mgr.pop(); }
protected:
  void renderTitle(Frame& f) { f.set(1, "*%s", title_); }
  const char* title_;
};

class GpsStatusScreen : public InfoScreen {
public:
  using InfoScreen::InfoScreen;
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

class AccGyroScreen : public InfoScreen {
public:
  using InfoScreen::InfoScreen;
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

class MagTempScreen : public InfoScreen {
public:
  using InfoScreen::InfoScreen;
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

class ImuCalibStatusScreen : public InfoScreen {
public:
  using InfoScreen::InfoScreen;
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

class AttEstScreen : public InfoScreen {
public:
  using InfoScreen::InfoScreen;
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

class HealthScreen : public InfoScreen {
public:
  using InfoScreen::InfoScreen;
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

class SleepScreen : public InfoScreen {
public:
  SleepScreen() : InfoScreen("Sleep") {}
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

// RA speed : RRcmd (click = edit) / RRate / Quit
class RaSpeedScreen : public Screen {
public:
  explicit RaSpeedScreen(const char* title) : title_(title), cursor_(3) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
private:
  const char* title_;
  MenuCursor cursor_;
  bool editing_ = false;
};

/************************************/
// Calibration (DES-CAL-01..05): the motor is stopped while these are on top
/************************************/
// Two choices on one line ("Apply / Discard", "Retry / Quit"); the dial picks one
class Choice {
public:
  void reset() { sel_ = 0; }
  void rotate(int delta) { if (delta > 0) sel_ = 1; else if (delta < 0) sel_ = 0; }
  bool first() const { return sel_ == 0; }
  void render(Frame& f, int row, const char* a, const char* b, char blink) const {
    f.set(row, "%c%s  %c%s", sel_ == 0 ? blink : ' ', a, sel_ == 1 ? blink : ' ', b);
  }
private:
  int sel_ = 0;
};

class GyroCalibScreen : public Screen {
public:
  void onEnter(UiContext& ctx) override { calib_.start(ctx.tel.now_ms); choice_.reset(); }
  void onRotate(int delta, UiContext& /*ctx*/) override { choice_.rotate(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void tick(UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  const cal::GyroCalibration& calibration() const { return calib_; }
private:
  cal::GyroCalibration calib_;
  Choice choice_;
};

class AccelCalibScreen : public Screen {
public:
  void onEnter(UiContext& ctx) override { calib_.start(ctx.tel.now_ms); choice_.reset(); }
  void onRotate(int delta, UiContext& /*ctx*/) override { choice_.rotate(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void tick(UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  const cal::AccelCalibration& calibration() const { return calib_; }
private:
  cal::AccelCalibration calib_;
  Choice choice_;
};

/************************************/
// Polar alignment (DES-PA-01..04)
/************************************/
class PolarAlignScreen : public Screen {
public:
  enum Item { CONFIRM = 0, BACK, ITEMS };
  static const int INFO_LINES = 6;
  explicit PolarAlignScreen(const char* title) : title_(title), cursor_(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); holding_ = false; ok_ = false; }
  void onRotate(int delta, UiContext& /*ctx*/) override { cursor_.move(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void tick(UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool ok() const { return ok_; }
private:
  const char* title_;
  PageCursor cursor_;
  bool holding_ = false, ok_ = false;
  unsigned long since_ms_ = 0;
};

/************************************/
// Target : position of the chosen target and of the tube, guidance, tracking
/************************************/
class TargetScreen : public Screen {
public:
  static constexpr float OFFSET_STEP = 1.0f;   // PWM duty per detent
  // Rows 2-7 are a window over INFO_LINES info lines + the items (PageCursor):
  // turning the dial scrolls the whole page.
  static const int INFO_LINES = 7;              // R.A., DEC., T.R.A., T.DEC., dRA, dDEC, ALT/AZ
  enum Item { TRACK = 0, RATE, OFFSET, BACK, ITEMS };
  TargetScreen() : cursor_(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
private:
  PageCursor cursor_;
  bool editing_ = false;
};

// Target menu : Moon, Mercury .. Saturn, Quit (scrolls)
class TargetMenuScreen : public Screen {
public:
  static const int ITEMS = 7;
  explicit TargetMenuScreen(const char* title) : title_(title), cursor_(ITEMS) {}
  void onEnter(UiContext& ctx) override;
  void onRotate(int delta, UiContext& /*ctx*/) override { cursor_.move(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
private:
  const char* title_;
  MenuCursor cursor_;
  TargetScreen target_;
};

/************************************/
// Settings (DES-CFG-05) and its Location / Time pages (DES-LOC-01..04)
//   the main loop does the flash work : UiState::settings_*_request
/************************************/
// Location : mode, manual position (click a value, turn, click for the next digit)
class LocationScreen : public Screen {
public:
  enum Item { MODE = 0, LAT, LON, ALT, FROM_GPS, SAVE, BACK, ITEMS };
  static const int INFO_LINES = 1;
  LocationScreen() : cursor_(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
  double step() const;                          // current digit of the edited value
private:
  int stepCount() const;
  PageCursor cursor_;
  bool editing_ = false;
  int step_ = 0;
};

// Time : manual date/time (local), UTC offset, Set clock
class TimeScreen : public Screen {
public:
  enum Item { YEAR = 0, MONTH, DAY, HOUR, MINUTE, SECOND, UTC_OFFSET, SET, BACK, ITEMS };
  static const int INFO_LINES = 1;
  static const int OFFSET_STEP_MIN = 15;
  TimeScreen() : cursor_(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& ctx) override;
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
private:
  PageCursor cursor_;
  bool editing_ = false;
  bool set_ = false;                             // "Set clock" done since entering
  int v_[6] = {2026, 1, 1, 0, 0, 0};             // local year, month, day, hour, minute, second
};

class SettingsScreen : public Screen {
public:
  enum Item { SET_RA_DUTY = 0, LOCATION, TIME, MOTOR_DIR, GUIDE_TOL, SAVE, RESET, BACK, ITEMS };
  static const int INFO_LINES = 2;               // Flash status, RAduty
  static constexpr float TOL_STEP = 0.1f;
  explicit SettingsScreen(const char* title) : title_(title), cursor_(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); confirm_reset_ = false; editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
private:
  const char* title_;
  PageCursor cursor_;
  bool confirm_reset_ = false;   // Reset needs a second click
  bool editing_ = false;         // Guide tol
  LocationScreen location_;
  TimeScreen time_;
};

#endif // _UI_SCREENS_H_
