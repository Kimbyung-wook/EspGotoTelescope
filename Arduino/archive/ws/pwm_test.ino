/************************************/
// Include Headers
/************************************/
// #include "addon.h"
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


unsigned long last_loop_time = micros();
unsigned long now_loop_time = micros();
unsigned long size_loop_time = 100*1000;

void setup(){
  pinMode(GPIO_D5, OUTPUT);
  pinMode(GPIO_D6, OUTPUT);
  analogWriteFreq(100);
  analogWriteRange(1000);
  analogWrite(GPIO_D5, 90);
  analogWrite(GPIO_D6, 90);


}
int duty_us = 40;
void loop(){

  duty_us = duty_us + 10;
  if(duty_us > 750)
    duty_us = 40;
  analogWrite(GPIO_D5, duty_us);
  analogWrite(GPIO_D6, duty_us);

  while(1){
    now_loop_time = micros();
    if(now_loop_time - last_loop_time > size_loop_time){
      last_loop_time = last_loop_time + size_loop_time;
      break;
    }
  }
}