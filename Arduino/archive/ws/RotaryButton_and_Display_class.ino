/************************************/
// Include Headers
/************************************/
#include "oled.h"
#include "esp_12f.h"
#include "rotary.h"
// Simple Scheduler
unsigned long last_loop_time  = micros();
unsigned long now_loop_time   = micros();
unsigned long size_loop_time  = 100*1000;
unsigned long delta_loop_time = 0;

/************************************/
// Display Settings
/************************************/
OLED* monitor           = new OLED();

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
//   sprintf(monitor->msg[5], "4. Intialize Buttons");
//   monitor->print();
//   sprintf(monitor->msg[5], "4. Buttons are ON");
//   monitor->print();
}

Rotary* rotary          = Rotary::getInstance();

void loop(){
  /************************************/
  // Display
  /************************************/
  sprintf(monitor->msg[2], "Direction %2d", rotary->getRotaryDirection());

  if (rotary->didButtonEvent() == true)
  {
    sprintf(monitor->msg[4],"Button click!");
    rotary->setButtonEventFalse();
  }
  else
  {
    sprintf(monitor->msg[4],"Button released!");
  }
  if(rotary->didRotaryEvent() == true){
    rotary->setRotaryEventFalse();
    rotary->setRotaryNeutral();
  }
  sprintf(monitor->msg[0], "button_cnt %4d",   rotary->getButtonCount());
  sprintf(monitor->msg[1], "rotary_cnt: %4d",  rotary->getRotaryCount());
//   sprintf(monitor->msg[5], "S1 %d S2 %d = %d",digitalRead(BUTTON_S1),
//                                               digitalRead(BUTTON_S2),
//                                               2*digitalRead(BUTTON_S1) + digitalRead(BUTTON_S2));
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