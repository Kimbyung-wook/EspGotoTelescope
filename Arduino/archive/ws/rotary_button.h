#pragma once

/************************************/
// Rotary Button 
/************************************/
// Rotary Encoder Inputs
#define BUTTON_KEY  GPIO_D4
#define BUTTON_S1   GPIO_D5
#define BUTTON_S2   GPIO_D6

volatile int rotary_pulse_s1 = 0;
volatile int rotary_pulse_s2 = 0;
volatile unsigned long rotary_last_pulse_s1 = 0;
volatile unsigned long rotary_last_pulse_s2 = 0;
volatile int rotary_state = 0;
volatile int rotary_state_prev = 0;
volatile int rotary_state_s1 = 0;
volatile int rotary_state_s2 = 0;
volatile int rotary_counter = 0;
volatile int rotary_right = false;
volatile int rotary_left = false;
volatile int rotary_event = true;

void setRotaryRight();
void setRotaryLeft();
void setRotaryNeutral();
void updateRotary();