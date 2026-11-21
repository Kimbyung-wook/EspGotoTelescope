// Minimal Arduino API stub for host (PC) unit tests.
#pragma once
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>

#define IRAM_ATTR
#define HIGH 1
#define LOW  0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define RAD_TO_DEG 57.295779513082320876798154814105

namespace stub {
  extern unsigned long now_ms;
  extern unsigned long now_us;
  extern int pin_level[32];
  extern int pin_analog[32];
}
inline unsigned long millis() { return stub::now_ms; }
inline unsigned long micros() { return stub::now_us; }
inline int  digitalRead(uint8_t pin) { return stub::pin_level[pin]; }
inline void digitalWrite(uint8_t pin, int v) { stub::pin_level[pin] = v; stub::pin_analog[pin] = -1; }
inline void analogWrite(uint8_t pin, int v) { stub::pin_analog[pin] = v; }
inline void pinMode(uint8_t, uint8_t) {}
inline void noInterrupts() {}
inline void interrupts() {}
