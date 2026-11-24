#include "imu.h"
#include "imu_ranges.h"
#include "../board_config.h"

namespace {

// board_config.h numbers <-> Adafruit_ICM20948 enums
icm20948_accel_range_t accelRange(int g) {
  switch (g) {
    case 2:  return ICM20948_ACCEL_RANGE_2_G;
    case 4:  return ICM20948_ACCEL_RANGE_4_G;
    case 8:  return ICM20948_ACCEL_RANGE_8_G;
    default: return ICM20948_ACCEL_RANGE_16_G;
  }
}
icm20948_gyro_range_t gyroRange(int dps) {
  switch (dps) {
    case 500:  return ICM20948_GYRO_RANGE_500_DPS;
    case 1000: return ICM20948_GYRO_RANGE_1000_DPS;
    case 2000: return ICM20948_GYRO_RANGE_2000_DPS;
    default:   return ICM20948_GYRO_RANGE_250_DPS;
  }
}
ak09916_data_rate_t magRate(int hz) {
  switch (hz) {
    case 0:   return AK09916_MAG_DATARATE_SHUTDOWN;
    case 1:   return AK09916_MAG_DATARATE_SINGLE;
    case 20:  return AK09916_MAG_DATARATE_20_HZ;
    case 50:  return AK09916_MAG_DATARATE_50_HZ;
    case 100: return AK09916_MAG_DATARATE_100_HZ;
    default:  return AK09916_MAG_DATARATE_10_HZ;
  }
}

}  // namespace

bool Imu::begin() {
  bool ok = device_.begin_I2C();
  device_.setAccelRange(accelRange(IMU_ACCEL_RANGE_G));
  device_.setGyroRange(gyroRange(IMU_GYRO_RANGE_DPS));
  device_.setMagDataRate(magRate(IMU_MAG_RATE_HZ));
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

// The ranges read back from the sensor
const char* Imu::accelRangeText() {
  switch (device_.getAccelRange()) {
    case ICM20948_ACCEL_RANGE_2_G:  return imuAccelRangeText(2);
    case ICM20948_ACCEL_RANGE_4_G:  return imuAccelRangeText(4);
    case ICM20948_ACCEL_RANGE_8_G:  return imuAccelRangeText(8);
    case ICM20948_ACCEL_RANGE_16_G: return imuAccelRangeText(16);
  }
  return imuAccelRangeText(-1);
}

const char* Imu::gyroRangeText() {
  switch (device_.getGyroRange()) {
    case ICM20948_GYRO_RANGE_250_DPS:  return imuGyroRangeText(250);
    case ICM20948_GYRO_RANGE_500_DPS:  return imuGyroRangeText(500);
    case ICM20948_GYRO_RANGE_1000_DPS: return imuGyroRangeText(1000);
    case ICM20948_GYRO_RANGE_2000_DPS: return imuGyroRangeText(2000);
  }
  return imuGyroRangeText(-1);
}

const char* Imu::magRateText() {
  switch (device_.getMagDataRate()) {
    case AK09916_MAG_DATARATE_SHUTDOWN: return imuMagRateText(0);
    case AK09916_MAG_DATARATE_SINGLE:   return imuMagRateText(1);
    case AK09916_MAG_DATARATE_10_HZ:    return imuMagRateText(10);
    case AK09916_MAG_DATARATE_20_HZ:    return imuMagRateText(20);
    case AK09916_MAG_DATARATE_50_HZ:    return imuMagRateText(50);
    case AK09916_MAG_DATARATE_100_HZ:   return imuMagRateText(100);
  }
  return imuMagRateText(-1);
}
