#include "rotary.h"
#include "esp_12f.h"

Rotary::Rotary()
{
  pinMode(BUTTON_S1,  INPUT);
  pinMode(BUTTON_S2,  INPUT);
  pinMode(BUTTON_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S1),  Rotary::isr_rotary_s1,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S2),  Rotary::isr_rotary_s2,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_KEY), Rotary::isr_button_click,  RISING);
}

//3201 3201
// 0 -> 4 (2(0+1)+2) -> 10 (2*(4+1)+0) -> 23 (2*(10+1)+1)
void Rotary::setRotaryRight(){
  rotary_counter++;
  rotary_left = true;
  rotary_right = false;
  rotary_event = true;
}
// 3102 3102
// 0 -> 3 (2*(0+1)+1) -> 8 (2*(3+1)+0) -> 20 (2*(8+1)+2)
void Rotary::setRotaryLeft(){
  rotary_counter--;
  rotary_left = false;
  rotary_right = true;
  rotary_event = true;
}
void Rotary::setRotaryNeutral(){
  rotary_event = false;
  rotary_left = false;
  rotary_right = false;
}
void Rotary::updateRotary(){
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
int Rotary::didButtonEvent()
{
    return did_button_clicked;
}
void Rotary::setButtonEventFalse()
{
    did_button_clicked = false;
}
void Rotary::setButtonEventTrue()
{
    did_button_clicked = true;
}

int Rotary::didRotaryEvent()
{
    return rotary_event;
}
void Rotary::setRotaryEventFalse()
{
    rotary_event = false;
}
void Rotary::setRotaryEventTrue()
{
    rotary_event = true;
}
int Rotary::getButtonCount()
{
    return button_counter;
}
int Rotary::getRotaryCount()
{
    return rotary_counter;
}
int Rotary::getRotaryDirection()
{
  return - rotary_left + rotary_right;
}
/************************************/
// Interrupt Service Routines
/************************************/
// CW  (+, R) : S1 true,   S2 Rising
// CCW (-, L) : S1 Rising, S2 true
void Rotary::isr_rotary_s1(){
  Rotary* rotary = Rotary::getInstance();
  // prevent being clicked over twice at once
  if(rotary->rotary_last_pulse_s1 - millis() < 30)
    rotary->rotary_pulse_s1 = false;
  else{
    rotary->rotary_pulse_s1 = true;
  }
  rotary->rotary_last_pulse_s1 = millis();

  // processing rotary action
  if(rotary->rotary_pulse_s1 == true){
    rotary->rotary_pulse_s1 = false;
    rotary->updateRotary();
  }
}

void Rotary::isr_rotary_s2(){
  Rotary* rotary = Rotary::getInstance();
  // prevent being clicked over twice at once
  if(rotary->rotary_last_pulse_s2 - millis() < 30)
    rotary->rotary_pulse_s2 = false;
  else{
    rotary->rotary_pulse_s2 = true;
  }
  rotary->rotary_last_pulse_s2 = millis();

  // processing rotary action
  if(rotary->rotary_pulse_s2 == true){
    rotary->rotary_pulse_s2 = false;
    rotary->updateRotary();
  }
}

// Button Interrupt Routine
void Rotary::isr_button_click(){
  Rotary* rotary = Rotary::getInstance();
  // prevent being clicked over twice at once
  if(rotary->button_last_press - millis() < 500)
    rotary->button_clicked = false;
  else
    rotary->button_clicked = true;
  rotary->button_last_press = millis();

  // processing button action
  // throw main loop the fact being clicked
  if(rotary->button_clicked == true){
    rotary->button_clicked = false;
    rotary->did_button_clicked = true;
    rotary->button_counter++;
  }
}