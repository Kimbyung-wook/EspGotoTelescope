#pragma once
#ifndef _FILTER_H_
#define _FILTER_H_

// Fixed-step 1st-order filters: the step dt is set in initialize().

// x_k = gamma * x_(k-1) + (1 - gamma) * u_k,  gamma = exp(-dt/tau)
class LPF {
public:
  void initialize(float tau, float dt);
  float update(float u);           // starts from the first input (no slow start-up)
  float state() const { return x_; }
private:
  float x_ = 0.0f;
  float gamma_ = 0.0f;
  bool  first_ = true;
};

// Angle from a low band (absolute, noisy: tilt from gravity) and a high band
// (rate, drifting: gyro), crossing over at tau:
//   x = LPF(x_low) + LPF(rate * tau)
class ComplementaryFilter {
public:
  void initialize(float tau, float dt);
  float update(float x_low, float rate_high);
private:
  float tau_ = 1.0f;
  LPF low_;
  LPF high_;
};

#endif // _FILTER_H_
