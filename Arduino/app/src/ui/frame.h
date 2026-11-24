#pragma once
#ifndef _UI_FRAME_H_
#define _UI_FRAME_H_
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/************************************/
// One OLED frame : 8 lines x 21 characters (+ null)
//   line 0    : status bar
//   line 1    : title
//   line 2..7 : content (6 lines)
/************************************/
struct Frame {
  static const int LINES = 8;
  static const int COLS  = 22;   // 21 characters + null
  static const int CONTENT_FIRST = 2;
  static const int CONTENT_LINES = 6;

  char lines[LINES][COLS];
  bool overflow = false;         // some text was cut at the line width (tests check it)

  Frame() { clear(); }
  void clear() { memset(lines, 0, sizeof(lines)); overflow = false; }

  // printf into one line, truncated to the line width
  void set(int row, const char* fmt, ...) __attribute__((format(printf, 3, 4))) {
    if (row < 0 || row >= LINES) return;
    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(lines[row], COLS, fmt, args);
    va_end(args);
    if (n >= COLS) overflow = true;
  }
  void blank(int from_row, int to_row = LINES - 1) {
    for (int r = from_row; r <= to_row; r++) set(r, " ");
  }
};

#endif // _UI_FRAME_H_
