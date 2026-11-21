#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "oled.h"
#include "esp_12f.h"

#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#define GPS_PORT Serial
#define DEBUG_PORT Serial1

// Simple Scheduler
float hz = 10.0f;
unsigned long last_loop_time  = micros();
unsigned long now_loop_time   = micros();
unsigned long size_loop_time  = 1000 * (1000 / hz);
unsigned long delta_loop_time = 0;

char led_onoff = true;

/************************************/
// Display Settings
/************************************/
OLED* monitor           = new OLED();
Adafruit_ICM20948 imu;

void setup()
{
  /************************************/
  // GPIO Settings
  /************************************/
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)

  /************************************/
  // Debug Setting
  /************************************/
  DEBUG_PORT.begin(115200);
  delay(500);
  
  int system_on = true;
  /************************************/
  // Display Initialization
  /************************************/
  int monitor_on  = monitor->setup_display();
  system_on = system_on & monitor_on;
  sprintf(monitor->msg[2], "1. Display is ON");
  monitor->print();


  /************************************/
  // IMU Setting - ICM 20948
  /************************************/
  if(!imu.begin_I2C()){
    sprintf(monitor->msg[3], "2. IMU Failed to find");
  }
  else{
    imu.setAccelRange(ICM20948_ACCEL_RANGE_16_G);
    switch (imu.getAccelRange()) {
    case ICM20948_ACCEL_RANGE_2_G:
      sprintf(monitor->msg[4], "Acc +-2G");
      break;
    case ICM20948_ACCEL_RANGE_4_G:
      sprintf(monitor->msg[4], "Acc +-4G");
      break;
    case ICM20948_ACCEL_RANGE_8_G:
      sprintf(monitor->msg[4], "Acc +-8G");
      break;
    case ICM20948_ACCEL_RANGE_16_G:
      sprintf(monitor->msg[4], "Acc +-16G");
      break;
    }
    imu.setGyroRange(ICM20948_GYRO_RANGE_250_DPS);
    switch (imu.getGyroRange()) {
    case ICM20948_GYRO_RANGE_250_DPS:
      sprintf(monitor->msg[5], "Gyro 250 dps");
      break;
    case ICM20948_GYRO_RANGE_500_DPS:
      sprintf(monitor->msg[5], "Gyro 500 dps");
      break;
    case ICM20948_GYRO_RANGE_1000_DPS:
      sprintf(monitor->msg[5], "Gyro 1000 dps");
      break;
    case ICM20948_GYRO_RANGE_2000_DPS:
      sprintf(monitor->msg[5], "Gyro 2000 dps");
      break;
    }
    uint16_t accel_divisor = imu.getAccelRateDivisor();
    float accel_rate = 1125 / (1.0 + accel_divisor);
    uint8_t gyro_divisor = imu.getGyroRateDivisor();
    float gyro_rate = 1100 / (1.0 + gyro_divisor);

    imu.setMagDataRate(AK09916_MAG_DATARATE_10_HZ);
    switch (imu.getMagDataRate()) {
    case AK09916_MAG_DATARATE_SHUTDOWN:
      sprintf(monitor->msg[6], "Mag Shutdown");
      break;
    case AK09916_MAG_DATARATE_SINGLE:
      sprintf(monitor->msg[6], "Mag Single/One shot");
      break;
    case AK09916_MAG_DATARATE_10_HZ:
      sprintf(monitor->msg[6], "Mag 10 Hz");
      break;
    case AK09916_MAG_DATARATE_20_HZ:
      sprintf(monitor->msg[6], "Mag 20 Hz");
      break;
    case AK09916_MAG_DATARATE_50_HZ:
      sprintf(monitor->msg[6], "Mag 50 Hz");
      break;
    case AK09916_MAG_DATARATE_100_HZ:
      sprintf(monitor->msg[6], "Mag 100 Hz");
      break;
    }
  }
  monitor->print();
  delay(500);

}

void loop() // run over and over again
{

  /************************************/
  // Frequency Manager
  // Outputs
  //   char disp_now : frequency indicator
  /************************************/
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t mag;
  sensors_event_t temp;
  imu.getEvent(&accel, &gyro, &temp, &mag);
  //                        123456789012345678901
  sprintf(monitor->msg[0], " Acc(m/s) |Gyro(dps)-");
  sprintf(monitor->msg[1], "%10.5f|%10.5f", accel.acceleration.x, gyro.gyro.x);
  sprintf(monitor->msg[2], "%10.5f|%10.5f", accel.acceleration.y, gyro.gyro.y);
  sprintf(monitor->msg[3], "%10.5f|%10.5f", accel.acceleration.z, gyro.gyro.z);
  sprintf(monitor->msg[4], "Mag X %10.5f", mag.magnetic.x);
  sprintf(monitor->msg[5], "Mag Y %10.5f", mag.magnetic.y);
  sprintf(monitor->msg[6], "Mag Z %10.5f", mag.magnetic.z);
  sprintf(monitor->msg[7], "Tmp %8.4f 'c", temp.temperature);
  monitor->print();

  /************************************/
  // Simple Scheduler & Blinker
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
        digitalWrite(LED_BUILTIN, led_onoff);  // turn the LED on (HIGH is the voltage level)
        led_onoff = !led_onoff;
      }
    }
  }
}