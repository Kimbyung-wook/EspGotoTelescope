#include "screen_manager.h"

ScreenManager::ScreenManager(Screen* root) {
  stack_[0] = root;
}

void ScreenManager::push(Screen* s, UiContext& ctx) {
  if (depth_ >= MAX_DEPTH) return;
  stack_[depth_++] = s;
  s->onEnter(ctx);
}

void ScreenManager::pop() {
  if (depth_ > 1) depth_--;
}

void ScreenManager::step(int rotate_delta, bool click, UiContext& ctx, Frame& f) {
  ctx.state.motor_inhibit = false;        // the top screen sets it again in tick() (DES-SYS-02)
  ctx.state.display_off = false;          // the Sleep screen sets it again in tick()
  if (rotate_delta != 0) top()->onRotate(rotate_delta, ctx);
  if (click)             top()->onClick(*this, ctx);
  top()->tick(ctx);
  top()->render(f, ctx, blink_);
  blink_ = (blink_ == '>') ? '-' : '>';
}
