#pragma once
#ifndef _UI_SCREEN_H_
#define _UI_SCREEN_H_
#include "frame.h"
#include "../app/app_state.h"

class ScreenManager;

/************************************/
// One screen of the menu tree.
// The manager calls, for the screen on top of the stack, every frame :
//   onRotate (if turned) -> onClick (if clicked) -> tick -> render
/************************************/
class Screen {
public:
  virtual ~Screen() {}
  virtual void onEnter(UiContext& /*ctx*/) {}                       // pushed on the stack
  virtual void onRotate(int /*delta*/, UiContext& /*ctx*/) {}
  virtual void onClick(ScreenManager& /*mgr*/, UiContext& /*ctx*/) {}
  virtual void tick(UiContext& /*ctx*/) {}
  // Fill lines 1..7. 'blink' is the blinking cursor character ('>' or '-').
  virtual void render(Frame& f, const UiContext& ctx, char blink) = 0;
};

#endif // _UI_SCREEN_H_
