#include "test_main.h"
#include "../app/src/input/rotary.h"

TEST(decoder_cw_transitions_count_up) {
  RotaryDecoder d;
  // state 0 -> 1 : CW
  CHECK(d.onEdge(1, 0, 1, 100) == +1);
  // state 1 -> 3 : no step, then 3 -> 2 : CW
  CHECK(d.onEdge(0, 1, 1, 200) == 0);
  CHECK(d.onEdge(1, 1, 0, 300) == +1);
}

TEST(decoder_ccw_transitions_count_down) {
  RotaryDecoder d;
  CHECK(d.onEdge(0, 0, 1, 100) == +1); // 0 -> 1
  CHECK(d.onEdge(1, 0, 0, 200) == -1); // 1 -> 0
  CHECK(d.onEdge(0, 1, 0, 300) == 0);  // 0 -> 2
  CHECK(d.onEdge(1, 1, 1, 400) == -1); // 2 -> 3
}

TEST(decoder_ignores_bounce_within_30ms_on_same_channel) {
  RotaryDecoder d;
  CHECK(d.onEdge(1, 0, 1, 100) == +1);
  CHECK(d.onEdge(1, 0, 0, 110) == 0);  // bounce, ignored and state kept
  CHECK(d.onEdge(1, 1, 0, 120) == 0);  // still bouncing (timer restarted)
  CHECK(d.onEdge(1, 0, 0, 200) == -1); // 1 -> 0 accepted
}

TEST(decoder_channels_debounce_independently) {
  RotaryDecoder d;
  CHECK(d.onEdge(1, 0, 1, 100) == +1);
  CHECK(d.onEdge(0, 1, 1, 105) == 0);  // other channel within 30ms is accepted (1 -> 3, no step)
  CHECK(d.onEdge(1, 1, 0, 140) == +1); // 3 -> 2
}

TEST(decoder_button_debounce_500ms) {
  RotaryDecoder d;
  CHECK(d.onButton(1000) == true);
  CHECK(d.onButton(1200) == false);
  CHECK(d.onButton(1800) == true);  // 600ms after the last press
}

TEST(decoder_handles_millis_wraparound) {
  RotaryDecoder d;
  CHECK(d.onButton(0xFFFFFF00UL) == true);
  CHECK(d.onButton(0x00000010UL) == false); // 0x110 ms later
  CHECK(d.onButton(0x00000300UL) == true);
}

TEST(rotary_isr_accumulates_count_and_clicks) {
  Rotary r(12, 13, 2);
  r.begin();
  stub::now_ms = 100; stub::pin_level[12] = 0; stub::pin_level[13] = 1; Rotary::isrS2(); // +1
  stub::now_ms = 200; stub::pin_level[12] = 1; stub::pin_level[13] = 1; Rotary::isrS1(); //  0
  stub::now_ms = 300; stub::pin_level[12] = 1; stub::pin_level[13] = 0; Rotary::isrS2(); // +1
  CHECK(r.count() == 2);

  CHECK(r.takeClick() == false);
  stub::now_ms = 1000; Rotary::isrButton();
  stub::now_ms = 1100; Rotary::isrButton(); // bounce
  CHECK(r.takeClick() == true);
  CHECK(r.takeClick() == false);           // consumed
  CHECK(r.clickCount() == 1);
}
