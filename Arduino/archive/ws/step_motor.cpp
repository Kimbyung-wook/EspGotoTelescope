//라이브러리
#include <Stepper.h>
#define GPIO_S1  8
#define GPIO_S2  9
#define GPIO_S3 10

#define GPIO_D0 16 // PWM
#define GPIO_D3  0 // PWM
#define GPIO_D4  2

#define GPIO_D5 14
#define GPIO_D6 12
#define GPIO_D7 13 // PWM
#define GPIO_D8 15 // PWM

#define STEPS 2048

#define STEP_IN1 GPIO_D5
#define STEP_IN2 GPIO_D6
#define STEP_IN3 GPIO_D7
#define STEP_IN4 GPIO_D8

//스텝모터 선언
Stepper stepMotor(STEPS, STEP_IN4, STEP_IN2, STEP_IN3, STEP_IN1);           

void setup() {
  pinMode(GPIO_D4, OUTPUT);
  digitalWrite(GPIO_D4, HIGH);  // turn the LED on (HIGH is the voltage level)
  //스텝 모터 동작 속도 10RPM
  stepMotor.setSpeed(5); 
}
void loop() {
  // 시계 방향으로 이동
  stepMotor.step(STEPS);
  digitalWrite(GPIO_D4, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(500);

  // 반시계 방향으로 이동
  stepMotor.step(-STEPS);
  digitalWrite(GPIO_D4, LOW);   // turn the LED off by making the voltage LOW
  delay(500);
}