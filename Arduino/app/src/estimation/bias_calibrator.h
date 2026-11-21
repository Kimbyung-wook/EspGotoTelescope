#pragma once
#ifndef _BIAS_CALIBRATOR_H_
#define _BIAS_CALIBRATOR_H_

/************************************/
// Gyro bias calibration with moving averages (mount must stay still)
//   Ready  0 ~  1 s : arithmetic mean of the samples (initial guess)
//   Coarse 1 ~  6 s : exponential moving average, alpha = 0.3
//   Fine   6 ~ 16 s : arithmetic mean of the fine-phase samples
//                     (an EMA with alpha = 0.01 would keep ~37 % of the coarse error)
//   Done   16 s ~   : bias is frozen
/************************************/
class GyroBiasCalibrator {
public:
  enum Phase { IDLE, READY, COARSE, FINE, DONE };

  static const unsigned long READY_MS  = 1000;
  static const unsigned long COARSE_MS = 6000;
  static const unsigned long FINE_MS   = 16000;

  void start(unsigned long now_ms) {
    start_ms_ = now_ms;
    n_ = 0;
    phase_ = READY;
    for (int i = 0; i < 3; i++) bias_[i] = 0.0f;
  }

  // w : raw gyro rate [deg/s]. Returns the current phase.
  Phase update(unsigned long now_ms, const float w[3]) {
    if (phase_ == IDLE || phase_ == DONE) return phase_;
    unsigned long elapsed = now_ms - start_ms_;
    if (elapsed < READY_MS) {
      phase_ = READY;
      mean(w);
    } else if (elapsed < COARSE_MS) {
      phase_ = COARSE;
      ema(w, 0.3f);
    } else if (elapsed < FINE_MS) {
      if (phase_ != FINE) { phase_ = FINE; n_ = 0; }
      mean(w);
    } else {
      phase_ = DONE;
    }
    return phase_;
  }

  Phase phase() const { return phase_; }
  bool  done()  const { return phase_ == DONE; }
  const float* bias() const { return bias_; }

  static const char* phaseText(Phase p) {
    switch (p) {
      case READY:  return "Calib Gyro Bias Ready";
      case COARSE: return "Calib Gyro Bias ing.1";
      case FINE:   return "Calib Gyro Bias ing.2";
      case DONE:   return "Calib Gyro Bias Done.";
      default:     return "Calib Gyro Bias";
    }
  }

private:
  void mean(const float w[3]) {
    n_++;
    for (int i = 0; i < 3; i++) bias_[i] += (w[i] - bias_[i]) / (float)n_;
  }
  void ema(const float w[3], float alpha) {
    if (n_ == 0) { for (int i = 0; i < 3; i++) bias_[i] = w[i]; n_ = 1; }
    for (int i = 0; i < 3; i++) bias_[i] = (1.0f - alpha) * bias_[i] + alpha * w[i];
  }

  Phase phase_ = IDLE;
  unsigned long start_ms_ = 0;
  unsigned long n_ = 0;
  float bias_[3] = {0.0f, 0.0f, 0.0f};
};

#endif // _BIAS_CALIBRATOR_H_
