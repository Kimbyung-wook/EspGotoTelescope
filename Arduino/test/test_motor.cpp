#include "test_main.h"
#include "../app/src/motor/ra_motor.h"

TEST(motor_output_direction_and_duty) {
  MotorOutput o = RaMotor::toOutput(0.0f);
  CHECK(o.in1 == LOW && o.in2 == LOW && o.duty == 0);
  o = RaMotor::toOutput(30.0f);
  CHECK(o.in1 == LOW && o.in2 == HIGH && o.duty == 30);
  o = RaMotor::toOutput(-12.7f);
  CHECK(o.in1 == HIGH && o.in2 == LOW && o.duty == 12);
}

TEST(motor_output_duty_is_clamped) {
  CHECK(RaMotor::toOutput(1000.0f).duty == 255);
  CHECK(RaMotor::toOutput(-300.0f).duty == 255);
}

TEST(motor_set_speed_drives_pins) {
  RaMotor m(15, 16, 0);
  m.begin();
  CHECK(stub::pin_analog[15] == 0);
  CHECK(stub::pin_level[16] == LOW && stub::pin_level[0] == LOW);
  m.setSpeed(-40.0f);
  CHECK(stub::pin_analog[15] == 40);
  CHECK(stub::pin_level[16] == HIGH && stub::pin_level[0] == LOW);
  CHECK_NEAR(m.speed(), -40.0, 1e-6);
}
