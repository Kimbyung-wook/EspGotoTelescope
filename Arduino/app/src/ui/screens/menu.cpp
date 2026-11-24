#include "menu.h"

void MenuScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  Screen* target = targets_[cursor_.selected()];
  if (target) mgr.push(target, ctx);
  else        mgr.pop();                     // Back
}

void MenuScreen::render(Frame& f, const UiContext& /*ctx*/, char blink) {
  if (root_) f.set(1, "%s", title_);
  else       f.set(1, "*%s", title_);
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int i = first + row;
    if (i >= cursor_.count()) { f.blank(Frame::CONTENT_FIRST + row, Frame::CONTENT_FIRST + row); continue; }
    f.set(Frame::CONTENT_FIRST + row, "%c%s", cursor_.mark(i, blink), labels_[i]);
  }
}
