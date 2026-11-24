#pragma once
#ifndef _UI_SCREENS_POLAR_ALIGN_H_
#define _UI_SCREENS_POLAR_ALIGN_H_
#include "widgets.h"

/************************************/
// Polar alignment (DES-PA-01..04)
/************************************/
class PolarAlignScreen : public PageScreen {
public:
  enum Item { CONFIRM = 0, BACK, ITEMS };
  static const int INFO_LINES = 6;
  explicit PolarAlignScreen(const char* title) : PageScreen(INFO_LINES, ITEMS), title_(title) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); holding_ = false; ok_ = false; }
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void tick(UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool ok() const { return ok_; }
private:
  const char* title_;
  bool holding_ = false, ok_ = false;
  unsigned long since_ms_ = 0;
};

#endif // _UI_SCREENS_POLAR_ALIGN_H_
