#pragma once
#ifndef _UI_SCREENS_RA_SPEED_H_
#define _UI_SCREENS_RA_SPEED_H_
#include "widgets.h"

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

#endif // _UI_SCREENS_RA_SPEED_H_
