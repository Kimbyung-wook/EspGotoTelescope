#pragma once
#ifndef _UTIL_STOPWATCH_US_H_
#define _UTIL_STOPWATCH_US_H_
#include <Arduino.h>

// Time of one piece of work [us]: start() ... stop(); last() until the next stop()
class StopwatchUs {
public:
  void start() { start_us_ = micros(); }
  unsigned long stop() { last_us_ = micros() - start_us_; return last_us_; }
  unsigned long last() const { return last_us_; }
private:
  unsigned long start_us_ = 0;
  unsigned long last_us_ = 0;
};

#endif // _UTIL_STOPWATCH_US_H_
