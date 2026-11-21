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

/************************************/
// Devices
/************************************/
#define GPS_PORT    Serial  // UART0 : shared with USB, no debug print
#define OLED_I2C_ADDRESS 0x3C

/************************************/
// Main loop
/************************************/
#define MAIN_LOOP_HZ 10.0f

#endif // _BOARD_CONFIG_H_
