#pragma once
#ifndef _ROTARY_DECODER_H_
#define _ROTARY_DECODER_H_
#include <Arduino.h>

/************************************/
// Rotary encoder decoding logic (no hardware access)
// - Edge debounce per channel (S1, S2)
// - Quadrature state = 2*S1 + S2
//   CW  (+1) : 3 -> 2, 0 -> 1
//   CCW (-1) : 1 -> 0, 2 -> 3
// - Push button debounce
// Methods are called from ISRs, so they are kept in IRAM.
/************************************/
class RotaryDecoder {
public:
  static const unsigned long EDGE_DEBOUNCE_MS   = 30;
  static const unsigned long BUTTON_DEBOUNCE_MS = 500;

  // channel : 0 = S1, 1 = S2. s1, s2 : pin levels read at the edge.
  // Returns +1 (CW), -1 (CCW) or 0 (no step / bounced).
  IRAM_ATTR int onEdge(int channel, int s1, int s2, unsigned long now_ms) {
    bool bounced = (now_ms - last_edge_ms_[channel]) < EDGE_DEBOUNCE_MS;
    last_edge_ms_[channel] = now_ms;
    if (bounced) return 0;

    int state = 2 * s1 + s2;
    int step = 0;
    if ((state_prev_ == 3 && state == 2) || (state_prev_ == 0 && state == 1)) step = +1;
    if ((state_prev_ == 1 && state == 0) || (state_prev_ == 2 && state == 3)) step = -1;
    state_prev_ = state;
    return step;
  }

  // Returns true when the press is accepted as a click.
  IRAM_ATTR bool onButton(unsigned long now_ms) {
    bool bounced = (now_ms - last_button_ms_) < BUTTON_DEBOUNCE_MS;
    last_button_ms_ = now_ms;
    return !bounced;
  }

private:
  unsigned long last_edge_ms_[2] = {0, 0};
  unsigned long last_button_ms_ = 0;
  int state_prev_ = 0;
};

#endif // _ROTARY_DECODER_H_
