#pragma once
#ifndef _UI_SCREENS_STATUS_H_
#define _UI_SCREENS_STATUS_H_
#include "widgets.h"

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
  void tick(UiContext& ctx) override { ctx.state.display_off = true; }   // OLED off
  void render(Frame& f, const UiContext& ctx, char blink) override;
};

#endif // _UI_SCREENS_STATUS_H_
