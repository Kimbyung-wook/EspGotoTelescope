#pragma once
#ifndef _ATTITUDE_ESTIMATOR_H_
#define _ATTITUDE_ESTIMATOR_H_
#include "filter.h"
#include "../sensors/imu_data.h"

/************************************/
// Roll/pitch from a complementary filter per axis
//   low band  : tilt from gravity (accelerometer)
//   high band : bias-corrected gyro rate
/************************************/
class AttitudeEstimator {
public:
  void begin(float tau_s, float dt_s);
  // d : body-frame IMU data with the gyro bias already removed
  void update(const ImuData& d, float dt_s);
  float rollDeg()  const { return roll_deg_; }
  float pitchDeg() const { return pitch_deg_; }

private:
  ComplementaryFilter roll_cf_;
  ComplementaryFilter pitch_cf_;
  float roll_deg_  = 0.0f;
  float pitch_deg_ = 0.0f;
};

#endif // _ATTITUDE_ESTIMATOR_H_
