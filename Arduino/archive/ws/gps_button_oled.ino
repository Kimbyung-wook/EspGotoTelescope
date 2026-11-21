/************************************/
// Include Headers
/************************************/
// #include "addon.h"
#define GPIO_S1  8
#define GPIO_S2  9
#define GPIO_S3 10
#define GPIO_D0 16 // PWM
#define GPIO_D3  0 // PWM
// #define GPIO_D4  2
#define GPIO_D5 14
#define GPIO_D6 12
#define GPIO_D7 13 // PWM
#define GPIO_D8 15 // PWM

#include "oled.h"
// #include "rotary_button.h"
// #include "mpu9250_custom.h"
#include "gps.h"
#include "tictok_us.h"

/************************************/
// Define Devices
/************************************/
#define GPS_PORT Serial
// #define DEBUG_PORT Serial1

Tictok_us* perform_main = new Tictok_us();
OLED* monitor           = new OLED();
GPS* gps                = new GPS(&GPS_PORT);

Tictok_us* perform_rest  = new Tictok_us();
unsigned long last_loop_time = micros();
unsigned long now_loop_time = micros();
unsigned long size_loop_time = 100*1000;

const int delay_ms = 100;
char led_onoff = true;
int cnt = 1;

/************************************/
// Rotary Button 
/************************************/
// Rotary Encoder Inputs
#define BUTTON_KEY  GPIO_D5
#define BUTTON_S1   GPIO_D6
#define BUTTON_S2   GPIO_D7

volatile int button_counter = 0;
volatile int button_s2 = 0;
volatile unsigned long last_rotary = 0;

volatile int button_clicked = false;
volatile unsigned long last_button_press = 0;


void setup() {
  int system_on = true;
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, led_onoff);

  /************************************/
  // Display Initialization
  /************************************/
  int monitor_on  = monitor->setup_display();
  system_on = system_on & monitor_on;
  sprintf(monitor->msg[2], "Display is ON");
  monitor->print();


  /************************************/
  // GPS Initialization
  /************************************/
  sprintf(monitor->msg[3], "Intialize GPS");
  monitor->print();
  int gps_on      = gps->setup_gps();
  system_on = system_on & gps_on;
  if(gps_on == true)    sprintf(monitor->msg[3], "GPS is ON");
  else                  sprintf(monitor->msg[3], "GPS is timeout");
  monitor->print();


  /************************************/
  // Rotary Button Settings
  /************************************/
	// Set encoder pins as inputs
  sprintf(monitor->msg[4], "Intialize Buttons");
	pinMode(BUTTON_S1,  INPUT);
	pinMode(BUTTON_S2,  INPUT);
	pinMode(BUTTON_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S1),  isr_rotary_counter, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_KEY), isr_button_click,   CHANGE);


  // Finalize system set-up
  if(system_on == false){
    while(1){
      sprintf(monitor->msg[1], "System failed %d", (int)led_onoff);
      monitor->print();
      led_onoff = !led_onoff;
      delay(delay_ms);
    }
  }
  delay(1000);
}





void loop() {
  perform_main->tic();
  
  led_onoff = !led_onoff;
  digitalWrite(LED_BUILTIN, led_onoff);
	
  int gps_health = gps->read_all();
  /************************************/
  // Display
  /************************************/
  // sprintf(monitor->msg[0], "T%5d - %4d %d", cnt++, Button->getRotateCnt(), Button->getButtonPress());
  sprintf(monitor->msg[0], "T%5d", cnt++);
  sprintf(monitor->msg[1], "C  %6d/R  %6d", perform_main->getdt(), perform_rest->getdt());
  sprintf(monitor->msg[2], "Dsp%6d/Gps%6d", monitor->perform_print.getdt(), gps->perform_read.getdt());

  sprintf(monitor->msg[3], "Cnt %4d / Button %d", button_counter, button_clicked);
  button_clicked = false;

  // GPS
  sprintf(monitor->msg[4], "Date: %02d/%02d/20%02d",     gps->gps_device->day, gps->gps_device->month, gps->gps_device->year);
  sprintf(monitor->msg[5], "Time: %02d:%02d:%02d.%03d",  gps->gps_device->hour, gps->gps_device->minute, gps->gps_device->seconds, gps->gps_device->milliseconds);
  sprintf(monitor->msg[6], "Fix %d Qual %d Sat %2d",      (int)gps->gps_device->fix, (int)gps->gps_device->fixquality, (int)gps->gps_device->satellites);
  sprintf(monitor->msg[7], "%c%9.6f%c%10.6f",            gps->gps_device->lat, gps->gps_device->latitudeDegrees,gps->gps_device->lon, gps->gps_device->longitudeDegrees);


  // DEBUG_PORT.println(monitor->msg[0]);
  // DEBUG_PORT.println(monitor->msg[1]);

  monitor->print();

	// Put in a slight delay to help debounce the reading
  perform_main->toc();
  
  perform_rest->tic();
  while(1){
    now_loop_time = micros();
    if(now_loop_time - last_loop_time > size_loop_time){
      last_loop_time = last_loop_time + size_loop_time;
      break;
    }
  }
  perform_rest->toc();
  
}


/************************************/
// Interrupt Service Routines
/************************************/
IRAM_ATTR void isr_rotary_counter(){
  if(last_rotary - millis() < 20)
    return;
  last_rotary = millis();
  
  button_s2 = digitalRead(BUTTON_S2);
  if (button_s2 == true)
    button_counter --;
  else // Encoder is rotating S2 so increment
    button_counter ++;
}

IRAM_ATTR void isr_button_click(){
  if(last_button_press - millis() < 500){
    button_clicked = true;
  }
  last_button_press = millis();
}