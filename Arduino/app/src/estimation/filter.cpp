#include "filter.h"
#include <math.h>

void LPF::initialize(float tau, float dt) {
  gamma_ = expf(-dt / tau);
  x_ = 0.0f;
  first_ = true;
}

float LPF::update(float u) {
  if (first_) {
    x_ = u;
    first_ = false;
  }
  x_ = gamma_ * x_ + (1.0f - gamma_) * u;
  return x_;
}

void ComplementaryFilter::initialize(float tau, float dt) {
  tau_ = tau;
  low_.initialize(tau, dt);
  high_.initialize(tau, dt);
}

float ComplementaryFilter::update(float x_low, float rate_high) {
  return low_.update(x_low) + high_.update(rate_high * tau_);
}
