#pragma once
#ifndef _OLED_H_
#define _OLED_H_
// Display Library
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "../util/tictok_us.h"
#include "../board_config.h"

/************************************/
// Display Settings
/************************************/
#define DISP_LINE_ROW 22
#define DISP_LINE_COLUMN 8

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS OLED_I2C_ADDRESS

class OLED{
  public: // Functions
    OLED();
    ~ OLED();
    
  public : // Variables
    int setup_display();
    void print();
    void setPower(bool on);   // SSD1306 display on/off (Sleep), sent only on a change

  public:
    char msg[DISP_LINE_COLUMN][DISP_LINE_ROW];
    Tictok_us perform_print;

  private:
    Adafruit_SSD1306 display_device;
    bool power_on = true;


};

#endif // _OLED_H_