#pragma once
#ifndef _IMU_H_
#define _IMU_H_
#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include "imu_data.h"

/************************************/
// ICM20948 (accel / gyro / AK09916 mag) on I2C
/************************************/
class Imu {
public:
  bool begin();              // returns false when the sensor is not found
  void read();               // updates data()
  const ImuData& data() const { return data_; }

  // Text for the boot screen
  const char* accelRangeText();
  const char* gyroRangeText();
  const char* magRateText();

private:
  Adafruit_ICM20948 device_;
  ImuData data_;
};

#endif // _IMU_H_
