// Minimal Arduino API for building the firmware UI on the PC simulator runner.
// Only what Arduino/app/src/ui and motor/ra_motor need; pins are no-ops because
// the runner reads the motor command from UiState via RaMotor::toOutput().
// (Arduino/test/stubs is for the unit tests; this copy belongs to the simulator.)
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <math.h>

#define HIGH 1
#define LOW  0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define IRAM_ATTR

inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, int) {}
inline void analogWrite(uint8_t, int) {}
