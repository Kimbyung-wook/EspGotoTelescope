// Timing helper kept out of the .ino: the Arduino preprocessor mangles templates there.
#pragma once
#include <Arduino.h>

template <typename Fn>
float timeUs(int n, Fn fn) {
  const uint32_t t0 = micros();
  for (int i = 0; i < n; i++) { fn(i); if ((i & 7) == 7) yield(); }
  return (micros() - t0) / (float)n;
}
