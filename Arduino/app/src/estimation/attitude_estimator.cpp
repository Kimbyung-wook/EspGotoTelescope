#include "attitude_estimator.h"

void AttitudeEstimator::begin(float tau_s, float dt_s) {
  roll_cf_.initialize(tau_s, dt_s);
  pitch_cf_.initialize(tau_s, dt_s);
  roll_deg_ = pitch_deg_ = 0.0f;
}

void AttitudeEstimator::update(const ImuData& d, float dt_s) {
  roll_deg_  = roll_cf_.update(rollFromAccelDeg(d),  d.wx, dt_s);
  pitch_deg_ = pitch_cf_.update(pitchFromAccelDeg(d), d.wy, dt_s);
}
