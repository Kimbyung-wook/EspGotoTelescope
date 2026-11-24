#pragma once
#ifndef _ROTARY_H_
#define _ROTARY_H_
#include <Arduino.h>
#include "esp_12f.h"
#include "singleton.hpp"

/************************************/
// Rotary Button 
/************************************/
// Rotary Encoder Inputs
#define BUTTON_KEY  GPIO_D4
#define BUTTON_S1   GPIO_D5
#define BUTTON_S2   GPIO_D6

class Rotary : public Singleton<Rotary>{
    public:
    Rotary();

    public:
    void setRotaryRight();
    void setRotaryLeft();
    void setRotaryNeutral();
    void updateRotary();
    int didButtonEvent();
    void setButtonEventFalse();
    void setButtonEventTrue();
    int  didRotaryEvent();
    void setRotaryEventFalse();
    void setRotaryEventTrue();
    int getButtonCount();
    int getRotaryCount();
    int getRotaryDirection();

    static void isr_rotary_s1();
    static void isr_rotary_s2();
    static void isr_button_click();

    private:
    int rotary_pulse_s1 = 0;
    int rotary_pulse_s2 = 0;
    unsigned long rotary_last_pulse_s1 = 0;
    unsigned long rotary_last_pulse_s2 = 0;
    int rotary_state = 0;
    int rotary_state_prev = 0;
    int rotary_state_s1 = 0;
    int rotary_state_s2 = 0;
    int rotary_counter = 0;
    int rotary_right = false;
    int rotary_left = false;
    int rotary_event = true;
    int button_counter = 0;
    int button_clicked = false;
    int did_button_clicked = false;
    unsigned long button_last_press = 0;
};
#endif // _ROTARY_H