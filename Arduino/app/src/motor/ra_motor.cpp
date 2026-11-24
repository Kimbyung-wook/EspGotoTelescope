#include "ra_motor.h"
#include <math.h>

RaMotor::RaMotor(uint8_t pin_pwm, uint8_t pin_in1, uint8_t pin_in2)
  : pin_pwm_(pin_pwm), pin_in1_(pin_in1), pin_in2_(pin_in2) {}

void RaMotor::begin() {
  pinMode(pin_pwm_, OUTPUT);
  pinMode(pin_in1_, OUTPUT);
  pinMode(pin_in2_, OUTPUT);
  setSpeed(0.0f);
}

MotorOutput RaMotor::toOutput(float speed) {
  MotorOutput out;
  if (speed > 0.0f)      { out.in1 = LOW;  out.in2 = HIGH; }
  else if (speed < 0.0f) { out.in1 = HIGH; out.in2 = LOW;  }
  else                   { out.in1 = LOW;  out.in2 = LOW;  }
  int duty = (int)fabsf(speed);
  out.duty = duty > PWM_MAX ? PWM_MAX : duty;
  return out;
}

void RaMotor::setSpeed(float speed) {
  speed_ = speed;
  MotorOutput out = toOutput(speed);
  analogWrite(pin_pwm_, out.duty);
  digitalWrite(pin_in1_, out.in1);
  digitalWrite(pin_in2_, out.in2);
}
