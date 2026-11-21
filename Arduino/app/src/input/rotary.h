#pragma once
#ifndef _ROTARY_H_
#define _ROTARY_H_
#include <Arduino.h>
#include "rotary_decoder.h"

/************************************/
// Rotary encoder + push button on interrupts.
// ISRs only accumulate the count and clicks;
// the main loop reads them with count() / takeClick().
/************************************/
class Rotary {
public:
  Rotary(uint8_t pin_s1, uint8_t pin_s2, uint8_t pin_key);

  void begin();            // pin modes + interrupts
  int  count();            // accumulated steps (CW +, CCW -)
  bool takeClick();        // true once per accepted click
  int  clickCount();       // total accepted clicks

  // Interrupt service routines (public for unit tests)
  static void isrS1();
  static void isrS2();
  static void isrButton();

private:
  void onEdge(int channel);
  void onButton();

  uint8_t pin_s1_, pin_s2_, pin_key_;
  RotaryDecoder decoder_;
  volatile int  count_ = 0;
  volatile int  clicks_ = 0;
  volatile bool click_pending_ = false;

  static Rotary* instance_;
};

#endif // _ROTARY_H_
