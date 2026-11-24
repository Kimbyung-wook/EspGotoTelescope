#pragma once
#ifndef _OLED_H_
#define _OLED_H_
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../util/stopwatch_us.h"

/************************************/
// SSD1306 128x64 on I2C: 8 text lines of 21 characters
/************************************/
class Oled {
public:
  static const int ROWS = 8;
  static const int COLS = 22;      // 21 characters + null

  Oled();
  bool begin();                    // false when the display does not answer
  void print(const char lines[ROWS][COLS]);
  void setPower(bool on);          // display on/off (Sleep), sent only on a change
  unsigned long printUs() const { return print_time_.last(); }

private:
  Adafruit_SSD1306 device_;
  StopwatchUs print_time_;
  bool power_on_ = true;
};

#endif // _OLED_H_
