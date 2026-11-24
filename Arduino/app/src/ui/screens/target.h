#pragma once
#ifndef _UI_SCREENS_TARGET_H_
#define _UI_SCREENS_TARGET_H_
#include "widgets.h"

/************************************/
// Target : position of the chosen target and of the tube, guidance, tracking
/************************************/
class TargetScreen : public PageScreen {
public:
  static constexpr float OFFSET_STEP = 1.0f;   // PWM duty per detent
  // Rows 2-7 are a window over INFO_LINES info lines + the items (PageScreen):
  // turning the dial scrolls the whole page.
  static const int INFO_LINES = 7;              // R.A., DEC., T.R.A., T.DEC., dRA, dDEC, ALT/AZ
  enum Item { TRACK = 0, RATE, OFFSET, BACK, ITEMS };
  TargetScreen() : PageScreen(INFO_LINES, ITEMS) {}
  void onEnter(UiContext& /*ctx*/) override { cursor_.reset(); editing_ = false; }
  void onRotate(int delta, UiContext& ctx) override;
  void onClick(ScreenManager& mgr, UiContext& ctx) override;
  void render(Frame& f, const UiContext& ctx, char blink) override;
  bool editing() const { return editing_; }
private:
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

#endif // _UI_SCREENS_TARGET_H_
