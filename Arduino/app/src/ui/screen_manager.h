#pragma once
#ifndef _UI_SCREEN_MANAGER_H_
#define _UI_SCREEN_MANAGER_H_
#include "screen.h"

/************************************/
// Stack of screens. push = enter, pop = back.
// Each screen keeps its own cursor, so going back returns to the same item.
/************************************/
class ScreenManager {
public:
  static const int MAX_DEPTH = 6;

  explicit ScreenManager(Screen* root);

  void push(Screen* s, UiContext& ctx);
  void pop();
  Screen* top() const { return stack_[depth_ - 1]; }
  int depth() const { return depth_; }

  // Apply one frame of input, then draw lines 1..7 of the top screen.
  void step(int rotate_delta, bool click, UiContext& ctx, Frame& f);
  char blink() const { return blink_; }

private:
  Screen* stack_[MAX_DEPTH];
  int depth_ = 1;
  char blink_ = '>';
};

#endif // _UI_SCREEN_MANAGER_H_
