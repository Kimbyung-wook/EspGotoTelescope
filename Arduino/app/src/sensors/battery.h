#pragma once
#ifndef _BATTERY_H_
#define _BATTERY_H_

/************************************/
// Supply voltage from the ESP8266 ADC (A0), pure C++ (the caller reads analogRead).
//   volts = raw * full_scale_v / 1023, then a first-order low-pass so the status bar
//   does not flicker with ADC noise and the motor PWM ripple.
/************************************/
class BatteryMonitor {
public:
  BatteryMonitor(float full_scale_v, float alpha) : full_scale_v_(full_scale_v), alpha_(alpha) {}

  // raw : 0..1023 from analogRead(A0)
  void update(int raw) {
    if (raw < 0) raw = 0;
    if (raw > 1023) raw = 1023;
    const float v = raw * full_scale_v_ / 1023.0f;
    volts_ = valid_ ? volts_ + alpha_ * (v - volts_) : v;   // first sample starts the filter
    valid_ = true;
  }
  bool  valid() const { return valid_; }
  float volts() const { return volts_; }

private:
  float full_scale_v_;
  float alpha_;
  float volts_ = 0.0f;
  bool  valid_ = false;
};

#endif // _BATTERY_H_
