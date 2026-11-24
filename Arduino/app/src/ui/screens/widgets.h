#pragma once
#ifndef _UI_SCREENS_WIDGETS_H_
#define _UI_SCREENS_WIDGETS_H_
#include "../screen_manager.h"

// Pieces the screens are built from: cursors, a two-way choice, and PageScreen
// (a page of info lines followed by items).

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

/************************************/
// Two choices on one line ("Apply / Discard", "Retry / Quit"); the dial picks one
/************************************/
class Choice {
public:
  void reset() { sel_ = 0; }
  void rotate(int delta) { if (delta > 0) sel_ = 1; else if (delta < 0) sel_ = 0; }
  bool first() const { return sel_ == 0; }
  void render(Frame& f, int row, const char* a, const char* b, char blink) const {
    f.set(row, "%c%s  %c%s", sel_ == 0 ? blink : ' ', a, sel_ == 1 ? blink : ' ', b);
  }
private:
  int sel_ = 0;
};

/************************************/
// Screen whose rows 2..7 are a window over 'info' lines, then 'items' lines
// (PageCursor). The dial scrolls the page; each screen draws one line at a time.
/************************************/
class PageScreen : public Screen {
public:
  PageScreen(int info, int items) : cursor_(info, items), info_(info) {}
  void onRotate(int delta, UiContext& /*ctx*/) override { cursor_.move(delta); }
protected:
  // line(row, n) for each content row: row = frame row, n = page line
  // (0 .. info-1 : info lines, then the items)
  template <class LineFn> void renderPage(LineFn line) const {
    const int first = cursor_.first();
    for (int r = 0; r < Frame::CONTENT_LINES; r++) line(Frame::CONTENT_FIRST + r, first + r);
  }
  int  itemAt(int n) const { return n - info_; }                 // < 0 : info line
  char markAt(int i, char blink) const { return i >= 0 ? cursor_.mark(i, blink) : ' '; }
  // '*' on the item being edited
  char editMarkAt(int i, char blink, bool editing) const {
    return editing && i == cursor_.selected() ? '*' : markAt(i, blink);
  }
  PageCursor cursor_;
private:
  int info_;
};

#endif // _UI_SCREENS_WIDGETS_H_
