#include "rotary_button.h"



//3201 3201
// 0 -> 4 (2(0+1)+2) -> 10 (2*(4+1)+0) -> 23 (2*(10+1)+1)
void setRotaryRight(){
  rotary_counter++;
  rotary_left = true;
  rotary_right = false;
  rotary_event = true;
}
// 3102 3102
// 0 -> 3 (2*(0+1)+1) -> 8 (2*(3+1)+0) -> 20 (2*(8+1)+2)
void setRotaryLeft(){
  rotary_counter--;
  rotary_left = false;
  rotary_right = true;
  rotary_event = true;
}
void setRotaryNeutral(){
  rotary_event = false;
  rotary_left = false;
  rotary_right = false;
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