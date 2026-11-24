// Boot screen lines, shared by setup() in app.ino and the UI simulator
// (tools/ui_simulator/runner/firmware_ui.cpp), so both show the same log.
// Order on the board: TITLE, display, IMU (+ the three range lines from
// sensors/imu_ranges.h), GPS, buttons, motor, settings (config::loadText), Wi-Fi.
#pragma once
#ifndef _APP_BOOT_TEXT_H_
#define _APP_BOOT_TEXT_H_
#include <stdio.h>
#include "../version.h"
#include "../board_config.h"

namespace boot {

constexpr const char* TITLE           = SIMPLEGOTO_NAME " v" SIMPLEGOTO_VERSION;
constexpr const char* IMU_PENDING     = "2. Intialize IMU";
constexpr const char* IMU_ON          = "2. IMU is ON";
constexpr const char* IMU_FAILED      = ">> IMU failed";
constexpr const char* GPS_PENDING     = "3. Intialize GPS";
constexpr const char* GPS_ON          = "3. GPS is ON";
constexpr const char* GPS_TIMEOUT     = ">> GPS is timeout";
constexpr const char* BUTTONS_PENDING = "4. Intialize Buttons";
constexpr const char* BUTTONS_ON      = "4. Buttons are ON";
constexpr const char* MOTOR_ON        = "5. GPIO/PWM is ON";
constexpr const char* WIFI_AP         = "7. WiFi AP " REMOTE_UI_AP_SSID;

// "1. Display ON 160MHz" (160 MHz expected: Tools > CPU Frequency)
inline void displayLine(char* out, size_t size, unsigned cpu_mhz) {
  snprintf(out, size, "1. Display ON %uMHz", cpu_mhz);
}

}  // namespace boot

#endif // _APP_BOOT_TEXT_H_
