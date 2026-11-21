/************************************/
// Include Headers
/************************************/
#include "oled.h"
#define GPIO_S1  8
#define GPIO_S2  9
#define GPIO_S3 10
#define GPIO_D0 16 // PWM
#define GPIO_D3  0 // PWM
#define GPIO_D4  2
#define GPIO_D5 14
#define GPIO_D6 12
#define GPIO_D7 13 // PWM
#define GPIO_D8 15 // PWM

void setRotaryRight();
void setRotaryLeft();
void setRotaryNeutral();
void updateRotary();

// Simple Scheduler
unsigned long last_loop_time  = micros();
unsigned long now_loop_time   = micros();
unsigned long size_loop_time  = 100*1000;
unsigned long delta_loop_time = 0;

/************************************/
// Display Settings
/************************************/
OLED* monitor           = new OLED();

/************************************/
// Rotary Button 
/************************************/
// Rotary Encoder Inputs
#define BUTTON_KEY  GPIO_D4
#define BUTTON_S1   GPIO_D6
#define BUTTON_S2   GPIO_D7

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
volatile int rotary_direction = 0;
volatile int rotary_left = false;
volatile int rotary_event = true;

volatile int button_counter = 0;
volatile int button_clicked = false;
volatile int did_button_clicked = false;
volatile unsigned long button_last_press = 0;

void setup() {
  int system_on = true;
  /************************************/
  // Display Initialization
  /************************************/
  int monitor_on  = monitor->setup_display();
  system_on = system_on & monitor_on;
  sprintf(monitor->msg[2], "1. Display is ON");
  monitor->print();

  /************************************/
  // Rotary Button Settings
  /************************************/
	// Set encoder pins as inputs
  sprintf(monitor->msg[5], "4. Intialize Buttons");
  monitor->print();
	pinMode(BUTTON_S1,  INPUT);
	pinMode(BUTTON_S2,  INPUT);
	pinMode(BUTTON_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S1),  isr_rotary_s1,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S2),  isr_rotary_s2,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_KEY), isr_button_click,  RISING);
  sprintf(monitor->msg[5], "4. Buttons are ON");
  monitor->print();
}

void loop() {	
  /************************************/
  // Display
  /************************************/
  sprintf(monitor->msg[2], "Direction %2d", rotary_direction);

  if (did_button_clicked == true){
    sprintf(monitor->msg[4],"Button click!");
    did_button_clicked = false;
  }
  else
    sprintf(monitor->msg[4],"Button released!");
  if(rotary_event == true){
    rotary_event = false;
    setRotaryNeutral();
  }
  sprintf(monitor->msg[0], "button_cnt %4d",   button_counter);
  sprintf(monitor->msg[1], "rotary_cnt: %4d",  rotary_counter);
  sprintf(monitor->msg[5], "S1 %d S2 %d = %d",digitalRead(BUTTON_S1),
                                              digitalRead(BUTTON_S2),
                                              2*digitalRead(BUTTON_S1) + digitalRead(BUTTON_S2));
  monitor->print();
  
  /************************************/
  // Simple Scheduler
  /************************************/
  while(1){
    now_loop_time = micros();
    // Clock Overflow Case, each time after about 70 mins
    if(now_loop_time < last_loop_time){
      last_loop_time = 0;
      break;
    }
    // Normal Case
    else{
      delta_loop_time = now_loop_time - last_loop_time;
      if(delta_loop_time > size_loop_time){
        last_loop_time = last_loop_time + size_loop_time;
        break;
      }
    }
  }
}
//3201 3201
// 0 -> 4 (2(0+1)+2) -> 10 (2*(4+1)+0) -> 23 (2*(10+1)+1)
void setRotaryRight(){
  rotary_counter++;
  rotary_direction = 1; // left
  rotary_event = true;
}
// 3102 3102
// 0 -> 3 (2*(0+1)+1) -> 8 (2*(3+1)+0) -> 20 (2*(8+1)+2)
void setRotaryLeft(){
  rotary_counter--;
  rotary_direction = -1; // right
  rotary_event = true;
}
void setRotaryNeutral(){
  rotary_event = false;
  rotary_direction = 0; // neutral
}
void updateRotary(){
  rotary_state_s1 = digitalRead(BUTTON_S1);
  rotary_state_s2 = digitalRead(BUTTON_S2);
  rotary_state = 2 * rotary_state_s1 + rotary_state_s2;
  if(rotary_state_prev == 3 && rotary_state == 2){
    setRotaryRight();
  }
  if(rotary_state_prev == 0 && rotary_state == 1){
    setRotaryRight();
  }

  if(rotary_state_prev == 1 && rotary_state == 0){
    setRotaryLeft();
  }
  if(rotary_state_prev == 2 && rotary_state == 3){
    setRotaryLeft();
  }
  rotary_state_prev = rotary_state;
}

/************************************/
// Interrupt Service Routines
/************************************/
// CW  (+, R) : S1 true,   S2 Rising
// CCW (-, L) : S1 Rising, S2 true
IRAM_ATTR void isr_rotary_s1(){
  // prevent being clicked over twice at once
  if(rotary_last_pulse_s1 - millis() < 30)
    rotary_pulse_s1 = false;
  else{
    rotary_pulse_s1 = true;
  }
  rotary_last_pulse_s1 = millis();

  // processing rotary action
  if(rotary_pulse_s1 == true){
    rotary_pulse_s1 = false;
    updateRotary();
  }
}

IRAM_ATTR void isr_rotary_s2(){
  // prevent being clicked over twice at once
  if(rotary_last_pulse_s2 - millis() < 30)
    rotary_pulse_s2 = false;
  else{
    rotary_pulse_s2 = true;
  }
  rotary_last_pulse_s2 = millis();

  // processing rotary action
  if(rotary_pulse_s2 == true){
    rotary_pulse_s2 = false;
    updateRotary();
  }
}

// Button Interrupt Routine
IRAM_ATTR void isr_button_click(){
  // prevent being clicked over twice at once
  if(button_last_press - millis() < 500)
    button_clicked = false;
  else
    button_clicked = true;
  button_last_press = millis();

  // processing button action
  // throw main loop the fact being clicked
  if(button_clicked == true){
    button_clicked = false;
    did_button_clicked = true;
    button_counter++;
  }
}