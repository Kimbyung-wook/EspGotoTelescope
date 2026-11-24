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

/************************************/
// Cursor over n items, shown in a window of 'rows' lines.
// The selected item stays at the bottom line while scrolling.
/************************************/
class MenuCursor {
public:
  explicit MenuCursor(int count, int rows = Frame::CONTENT_LINES) : count_(count), rows_(rows) {}
  void move(int delta) {
    sel_ += delta;
    if (sel_ < 0) sel_ = 0;
    if (sel_ > count_ - 1) sel_ = count_ - 1;
  }
  void reset() { sel_ = 0; }
  int  selected() const { return sel_; }
  int  first() const { return sel_ > rows_ - 1 ? sel_ - (rows_ - 1) : 0; }
  int  count() const { return count_; }
  // cursor character for item i
  char mark(int i, char blink) const { return i == sel_ ? blink : ' '; }

private:
  int count_;
  int rows_;
  int sel_ = 0;
};

// Scroll window over 'total' lines (info lines, then items): first line shown so
// that 'cursor_line' is visible, on the last row at most.
inline int windowFirst(int total, int cursor_line, int rows = Frame::CONTENT_LINES) {
  int first = cursor_line - (rows - 1);
  if (first > total - rows) first = total - rows;
  if (first < 0) first = 0;
  return first;
}

/************************************/
// Page of 'info' lines followed by 'items' selectable lines. The window follows
// the cursor; turning up from the first item scrolls on up to the first info
// line (no item is selected while doing so).
/************************************/
class PageCursor {
public:
  PageCursor(int info, int items) : info_(info), items_(items) {}
  void move(int delta) {
    for (; delta < 0; delta++) {
      if (sel_ > 0) sel_--;
      else if (up_ < maxUp()) up_++;
    }
    for (; delta > 0; delta--) {
      if (up_ > 0) up_--;
      else if (sel_ < items_ - 1) sel_++;
    }
  }
  void reset() { sel_ = 0; up_ = 0; }
  int  selected() const { return sel_; }
  bool viewingInfo() const { return up_ > 0; }   // scrolled above the items: a click does nothing
  int  first() const { return windowFirst(info_ + items_, info_ + sel_) - up_; }
  char mark(int i, char blink) const { return (up_ == 0 && i == sel_) ? blink : ' '; }
private:
  int maxUp() const { return windowFirst(info_ + items_, info_); }
  int info_, items_;
  int sel_ = 0, up_ = 0;
};

#endif // _UI_SCREEN_H_
