#pragma once
#ifndef _UI_SCREENS_H_
#define _UI_SCREENS_H_
#include "screen_manager.h"
#include "../estimation/bias_calibrator.h"

/************************************/
// Screens of docs/UI.md
/************************************/

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

class MoonAxisScreen : public InfoScreen {
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

// Gyro bias calibration : keep the mount still for 16 s
class GyroBiasCalibScreen : public Screen {
public:
  void onEnter(UiContext& ctx) override { calib_.start(ctx.tel.now_ms); }
  void tick(UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& /*ctx*/) override { mgr.pop(); }
  void render(Frame& f, const UiContext& ctx, char blink) override;
private:
  GyroBiasCalibrator calib_;
};

// Do IMU Calib. menu
class ImuCalibMenuScreen : public Screen {
public:
  explicit ImuCalibMenuScreen(const char* title) : title_(title), cursor_(6) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); }
  void onRotate(int delta, UiContext& /*ctx*/) override { cursor_.move(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
private:
  const char* title_;
  MenuCursor cursor_;
  GyroBiasCalibScreen gyro_bias_;
};

// Planet Tracking : position of the chosen target and of the tube, tracking on/off,
// speed offset (click Offset, turn the dial, click again)
class TargetScreen : public Screen {
public:
  static constexpr float OFFSET_STEP = 1.0f;   // PWM duty per detent
  TargetScreen() : cursor_(3) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
private:
  MenuCursor cursor_;   // 0 Track on/off, 1 Offset, 2 Back
  bool editing_ = false;
};

// Planet Tracking menu : Moon, Mercury .. Saturn, Quit (scrolls)
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

// Main menu with 11 items in a 6-line scroll window
class MainMenuScreen : public Screen {
public:
  static const int ITEMS = 11;
  MainMenuScreen(const char* const labels[ITEMS], Screen* const targets[ITEMS]);
  void onRotate(int delta, UiContext& /*ctx*/) override { cursor_.move(delta); }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  int selected() const { return cursor_.selected(); }
private:
  const char* const* labels_;
  Screen* const* targets_;
  MenuCursor cursor_;
};

#endif // _UI_SCREENS_H_
