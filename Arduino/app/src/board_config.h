#pragma once
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_
/************************************/
// Board : NodeMCU v3 (ESP-12F, ESP8266)
// All pin assignments and fixed settings live here.
/************************************/

/************************************/
// ESP-12F GPIO numbers of NodeMCU pins
/************************************/
#define GPIO_S1  8
#define GPIO_S2  9
#define GPIO_S3 10
#define GPIO_D0 16 // no interrupt
#define GPIO_D1  5 // I2C SCL
#define GPIO_D2  4 // I2C SDA
#define GPIO_D3  0 // boot strap : must be HIGH at boot
#define GPIO_D4  2 // boot strap : must be HIGH at boot, on-board LED
#define GPIO_D5 14
#define GPIO_D6 12
#define GPIO_D7 13
#define GPIO_D8 15 // boot strap : must be LOW at boot

/************************************/
// Rotary encoder with push button
/************************************/
#define BUTTON_KEY  GPIO_D4
#define BUTTON_S1   GPIO_D6
#define BUTTON_S2   GPIO_D7

/************************************/
// RA axis DC motor (H-bridge)
/************************************/
#define MOTOR_PWM   GPIO_D8 // speed (enable)
#define MOTOR_IN1   GPIO_D0 // direction
#define MOTOR_IN2   GPIO_D3 // direction
// RRcmd (PWM duty) that turns the RA axis at the sidereal rate (15.041"/s).
// Find it on the 1RA Speed screen by following a star. 0 = not calibrated:
// Planet Tracking then computes the rate but does not drive the motor.
#define RA_DUTY_AT_SIDEREAL 0.0f

// ICM20948 on the telescope tube: which IMU body axis (1 = x, 2 = y, 3 = z,
// negative = opposite direction) runs along the optical axis and along the Dec axis.
// (assumed) Check: point the tube at the celestial pole -> Planet Tracking "T.DEC." ~ +90.
#define IMU_TUBE_OPTICAL_AXIS  1
#define IMU_TUBE_DEC_AXIS      2

/************************************/
// Devices
/************************************/
#define GPS_PORT    Serial  // UART0 : shared with USB, no debug print
#define OLED_I2C_ADDRESS 0x3C

/************************************/
// Main loop
/************************************/
#define MAIN_LOOP_HZ 10.0f

/************************************/
// Remote UI : phone browser over Wi-Fi (docs/REMOTE_UI_PLAN.md)
//   0 = Wi-Fi is turned off at boot. 1 = SoftAP + web page + WebSocket.
//   Override without editing: --build-property "compiler.cpp.extra_flags=-DREMOTE_UI_ENABLE=1"
//   Needs the "WebSockets" library (Markus Sattler, links2004) when 1.
/************************************/
#ifndef REMOTE_UI_ENABLE
#define REMOTE_UI_ENABLE 0
#endif
#define REMOTE_UI_AP_SSID    "Telescope"
#define REMOTE_UI_AP_PASS    "telescope"  // WPA2, at least 8 characters
#define REMOTE_UI_AP_CHANNEL 6
#define REMOTE_UI_TX_DBM     10.0f        // 0 .. 20.5 dBm (lower = less current and Mag noise)
#define REMOTE_UI_HTTP_PORT  80           // page : http://192.168.4.1/
#define REMOTE_UI_WS_PORT    81           // WebSocket : ws://192.168.4.1:81/

#endif // _BOARD_CONFIG_H_
