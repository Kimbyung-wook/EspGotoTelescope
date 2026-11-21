#pragma once
#ifndef _IMU_DATA_H_
#define _IMU_DATA_H_
#include <math.h>

/************************************/
// IMU measurement in the telescope body frame
/************************************/
struct ImuData {
  float ax = 0, ay = 0, az = 0;   // [m/s^2]
  float wx = 0, wy = 0, wz = 0;   // [deg/s]
  float mx = 0, my = 0, mz = 0;   // [uT]
  float temperature = 0;          // [degC]
};

// Sensor frame (ICM20948 as mounted) -> body frame.
// Inputs are in the sensor library units : accel [m/s^2], gyro [rad/s], mag [uT].
inline ImuData imuSensorToBody(const float acc[3], const float gyro_radps[3],
                               const float mag[3], float temperature) {
  const float RAD2DEG = 57.29577951308232f;
  ImuData d;
  d.ax = -acc[0];
  d.ay =  acc[1];
  d.az =  acc[2];
  d.wx =  gyro_radps[0] * RAD2DEG;
  d.wy = -gyro_radps[1] * RAD2DEG;
  d.wz = -gyro_radps[2] * RAD2DEG;
  d.mx = mag[0];
  d.my = mag[1];
  d.mz = mag[2];
  d.temperature = temperature;
  return d;
}

// Gyro rate with the calibrated bias removed
inline ImuData withoutGyroBias(const ImuData& d, const float bias_dps[3]) {
  ImuData c = d;
  c.wx -= bias_dps[0];
  c.wy -= bias_dps[1];
  c.wz -= bias_dps[2];
  return c;
}

/************************************/
// Tilt from gravity (valid when the mount is not accelerating)
/************************************/
inline float accelNorm(const ImuData& d) {
  return sqrtf(d.ax * d.ax + d.ay * d.ay + d.az * d.az);
}
inline float rollFromAccelDeg(const ImuData& d) {
  return atan2f(d.ay, d.az) * 57.29577951308232f;
}
inline float pitchFromAccelDeg(const ImuData& d) {
  float g = accelNorm(d);
  if (g <= 0.0f) return 0.0f;
  return asinf(-d.ax / g) * 57.29577951308232f;
}

#endif // _IMU_DATA_H_
