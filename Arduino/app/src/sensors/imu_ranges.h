#pragma once
#ifndef _IMU_RANGES_H_
#define _IMU_RANGES_H_

/************************************/
// ICM20948 range texts for the boot screen, from the plain numbers
// (board_config.h IMU_ACCEL_RANGE_G, IMU_GYRO_RANGE_DPS, IMU_MAG_RATE_HZ).
// Pure C++: the UI simulator shows the same lines as the board.
/************************************/
inline const char* imuAccelRangeText(int g) {
  switch (g) {
    case 2:  return "Acc +-2G";
    case 4:  return "Acc +-4G";
    case 8:  return "Acc +-8G";
    case 16: return "Acc +-16G";
  }
  return "Acc ?";
}

inline const char* imuGyroRangeText(int dps) {
  switch (dps) {
    case 250:  return "Gyro 250 dps";
    case 500:  return "Gyro 500 dps";
    case 1000: return "Gyro 1000 dps";
    case 2000: return "Gyro 2000 dps";
  }
  return "Gyro ?";
}

// 0 = shutdown, 1 = single shot
inline const char* imuMagRateText(int hz) {
  switch (hz) {
    case 0:   return "Mag Shutdown";
    case 1:   return "Mag Single/One shot";
    case 10:  return "Mag 10 Hz";
    case 20:  return "Mag 20 Hz";
    case 50:  return "Mag 50 Hz";
    case 100: return "Mag 100 Hz";
  }
  return "Mag ?";
}

#endif // _IMU_RANGES_H_
