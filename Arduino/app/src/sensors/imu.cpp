#include "imu.h"

bool Imu::begin() {
  bool ok = device_.begin_I2C();
  device_.setAccelRange(ICM20948_ACCEL_RANGE_16_G);
  device_.setGyroRange(ICM20948_GYRO_RANGE_250_DPS);
  device_.setMagDataRate(AK09916_MAG_DATARATE_10_HZ);
  return ok;
}

void Imu::read() {
  sensors_event_t accel, gyro, mag, temp;
  device_.getEvent(&accel, &gyro, &temp, &mag);
  const float a[3] = {accel.acceleration.x, accel.acceleration.y, accel.acceleration.z};
  const float w[3] = {gyro.gyro.x, gyro.gyro.y, gyro.gyro.z};
  const float m[3] = {mag.magnetic.x, mag.magnetic.y, mag.magnetic.z};
  data_ = imuSensorToBody(a, w, m, temp.temperature);
}

const char* Imu::accelRangeText() {
  switch (device_.getAccelRange()) {
    case ICM20948_ACCEL_RANGE_2_G:  return "Acc +-2G";
    case ICM20948_ACCEL_RANGE_4_G:  return "Acc +-4G";
    case ICM20948_ACCEL_RANGE_8_G:  return "Acc +-8G";
    case ICM20948_ACCEL_RANGE_16_G: return "Acc +-16G";
  }
  return "Acc ?";
}

const char* Imu::gyroRangeText() {
  switch (device_.getGyroRange()) {
    case ICM20948_GYRO_RANGE_250_DPS:  return "Gyro 250 dps";
    case ICM20948_GYRO_RANGE_500_DPS:  return "Gyro 500 dps";
    case ICM20948_GYRO_RANGE_1000_DPS: return "Gyro 1000 dps";
    case ICM20948_GYRO_RANGE_2000_DPS: return "Gyro 2000 dps";
  }
  return "Gyro ?";
}

const char* Imu::magRateText() {
  switch (device_.getMagDataRate()) {
    case AK09916_MAG_DATARATE_SHUTDOWN: return "Mag Shutdown";
    case AK09916_MAG_DATARATE_SINGLE:   return "Mag Single/One shot";
    case AK09916_MAG_DATARATE_10_HZ:    return "Mag 10 Hz";
    case AK09916_MAG_DATARATE_20_HZ:    return "Mag 20 Hz";
    case AK09916_MAG_DATARATE_50_HZ:    return "Mag 50 Hz";
    case AK09916_MAG_DATARATE_100_HZ:   return "Mag 100 Hz";
  }
  return "Mag ?";
}
