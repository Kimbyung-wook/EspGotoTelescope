#pragma once
#ifndef _UI_SCREENS_MENU_H_
#define _UI_SCREENS_MENU_H_
#include "widgets.h"

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

#endif // _UI_SCREENS_MENU_H_
