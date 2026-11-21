#pragma once
#ifndef _UI_CONTEXT_H_
#define _UI_CONTEXT_H_
#include "../sensors/imu_data.h"

/************************************/
// Values the UI shows (filled by the main loop every frame)
/************************************/
struct Telemetry {
  unsigned long now_ms = 0;

  // GPS
  int  year = 0, month = 0, day = 0;
  int  hour = 0, minute = 0, seconds = 0, milliseconds = 0;
  int  fixquality = 0, satellites = 0;
  char lat_hemi = ' ', lon_hemi = ' ';
  double latitude = 0.0, longitude = 0.0;   // [deg]

  // IMU
  ImuData imu;
  float roll_acc_deg = 0.0f, pitch_acc_deg = 0.0f;

  // Power
  float voltage = 12.60f;                   // not measured yet

  // Health [us]
  float loop_hz = 10.0f;
  unsigned long loop_us = 0, main_us = 0, disp_us = 0, gps_us = 0, imu_us = 0, est_us = 0;
};

/************************************/
// Values the UI changes (read by the main loop)
/************************************/
struct UiState {
  float ra_speed_cmd = 0.0f;                // RA motor speed command
  float gyro_bias[3] = {0.0f, 0.0f, 0.0f};  // [deg/s]
  float accel_bias[3] = {0.0f, 0.0f, 0.0f}; // [m/s^2] (calibration not implemented)
};

struct UiContext {
  Telemetry tel;
  UiState   state;
};

#endif // _UI_CONTEXT_H_
