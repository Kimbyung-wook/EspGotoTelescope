#pragma once
#ifndef _RA_MOTOR_H_
#define _RA_MOTOR_H_
#include <Arduino.h>

/************************************/
// RA axis DC motor through an H-bridge
//   speed > 0 : IN1 LOW,  IN2 HIGH
//   speed < 0 : IN1 HIGH, IN2 LOW
//   speed = 0 : IN1 LOW,  IN2 LOW (stop)
//   PWM duty  : |speed|, limited to 0..PWM_MAX
/************************************/
struct MotorOutput {
  int in1;
  int in2;
  int duty;
};

class RaMotor {
public:
  static const int PWM_MAX = 255;

  RaMotor(uint8_t pin_pwm, uint8_t pin_in1, uint8_t pin_in2);
  void begin();                  // outputs, motor stopped
  void setSpeed(float speed);    // speed command (PWM duty units, signed)
  float speed() const { return speed_; }

  static MotorOutput toOutput(float speed);   // pure mapping, unit tested

private:
  uint8_t pin_pwm_, pin_in1_, pin_in2_;
  float speed_ = 0.0f;
};

#endif // _RA_MOTOR_H_
