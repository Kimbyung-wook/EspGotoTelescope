/************************************/
// Include Headers
/************************************/
// #include "addon.h"
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

#include "oled.h"
// #include "rotary_button.h"
// #include "mpu9250_custom.h"
#include "gps.h"
#include "tictok_us.h"
#include "mpu6500.h" // 6.0.3
#include "filter.h"
#include "Servo.h"

/************************************/
// Define Devices
/************************************/
#define GPS_PORT Serial
// #define DEBUG_PORT Serial1

Tictok_us* perform_main = new Tictok_us();
OLED* monitor           = new OLED();
GPS* gps                = new GPS(&GPS_PORT);
bfs::Mpu6500* imu       = new bfs::Mpu6500();
Tictok_us* perform_rest = new Tictok_us();
Tictok_us* perform_imu  = new Tictok_us();
Tictok_us* perform_est  = new Tictok_us();
Tictok_us* perform_pwm  = new Tictok_us();

float base_hz = 20.0f;
float gps_hz = 5.0f;
float imu_hz = 20.0f;
float display_hz = 5.0f;
float 
unsigned long last_loop_time  = micros();
unsigned long now_loop_time   = micros();
unsigned long size_loop_time  = 200*1000;
unsigned long delta_loop_time = 0;

float dt;
ComplementaryFilter* Phi_cf = new ComplementaryFilter();
ComplementaryFilter* Theta_cf = new ComplementaryFilter();

char led_onoff = true;
const int delay_ms = (int) size_loop_time/1000;
int cnt = 1;

Servo PWM1;
Servo PWM2;
int duty_us = 60;
/************************************/
// Rotary Button 
/************************************/
// Rotary Encoder Inputs
#define BUTTON_KEY  GPIO_D4
#define BUTTON_S1   GPIO_D7
#define BUTTON_S2   GPIO_D8

volatile int button_counter = 0;
volatile int button_s1 = 0;
volatile int button_s2 = 0;
volatile int rotary_right = false;
volatile int rotary_left = false;
volatile unsigned long rotary_last_press_s1 = 0;
volatile unsigned long rotary_last_press_s2 = 0;

volatile int button_clicked = false;
volatile unsigned long button_last_press = 0;

#define SHOW_GPS 1
#define SHOW_IMU 2
#define SHOW_EST 3
#define SHOW_PERFORM 4
#define SHOW_START SHOW_GPS
#define SHOW_END SHOW_PERFORM

int menu_mode = SHOW_GPS;

float y_prev = 0.0f;
float u_prev = 0.0f;
float tau = 0.100000f;

void setup() {
  dt = (((float)delay_ms)/1000.0f);
  Phi_cf->initialize(tau, dt);
  Theta_cf->initialize(tau, dt);

  int system_on = true;

  Wire.setClock(400*1000);
  /************************************/
  // Display Initialization
  /************************************/
  int monitor_on  = monitor->setup_display();
  system_on = system_on & monitor_on;
  sprintf(monitor->msg[2], "1. Display is ON");
  monitor->print();


  /************************************/
  // Display Initialization
  /************************************/
  sprintf(monitor->msg[3], "2. Intialize IMU");
  monitor->print();
  /* I2C bus,  0x68 address */
  imu->Config(&Wire, bfs::Mpu6500::I2C_ADDR_PRIM);
  int imu_on = imu->Begin()
              // Set the accel range to 16G by default
            && imu->ConfigAccelRange(bfs::Mpu6500::ACCEL_RANGE_4G)
              // Set the gyro range to 2000DPS by default
            && imu->ConfigGyroRange(bfs::Mpu6500::GYRO_RANGE_250DPS)
              // Set the DLPF to 184HZ by default
            && imu->ConfigDlpfBandwidth(bfs::Mpu6500::DLPF_BANDWIDTH_92HZ)
              // Set the sample rate divider
            && imu->ConfigSrd(19);                              
  system_on = system_on & imu_on;
  if(imu_on == true)    sprintf(monitor->msg[3], "2. IMU is ON");
  else                  sprintf(monitor->msg[3], ">> IMU failed");
  monitor->print();


  /************************************/
  // GPS Initialization
  /************************************/
  sprintf(monitor->msg[4], "Intialize GPS");
  monitor->print();
  int gps_on      = gps->setup_gps();
  system_on = system_on & gps_on;
  if(gps_on == true)    sprintf(monitor->msg[4], "3. GPS is ON");
  else                  sprintf(monitor->msg[4], ">> GPS is timeout");
  monitor->print();


  /************************************/
  // Rotary Button Settings
  /************************************/
	// Set encoder pins as inputs
  sprintf(monitor->msg[5], "4. Intialize Buttons");
  monitor->print();
	pinMode(BUTTON_S1,  INPUT);
	pinMode(BUTTON_S2,  INPUT);
	pinMode(BUTTON_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S1),  isr_rotary_s1, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(BUTTON_S2),  isr_rotary_s2, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_KEY), isr_button_click,   RISING);
  sprintf(monitor->msg[5], "4. Buttons are ON");
  monitor->print();

  pinMode(GPIO_D5, OUTPUT);
  pinMode(GPIO_D6, OUTPUT);
  analogWriteFreq(100);
  analogWriteRange(1000);
  analogWrite(GPIO_D6, 90);
  // PWM1.attach(GPIO_D5, 600, 2100);
  // PWM2.attach(GPIO_D6, 600, 2100);
  // PWM1.writeMicroseconds(20000);
  // PWM2.writeMicroseconds(20000);


  // Finalize system set-up
  if(system_on == false){
    while(1){
      sprintf(monitor->msg[1], "System failed %d", (int)led_onoff);
      monitor->print();
      led_onoff = !led_onoff;
      delay(delay_ms);
    }
  }
  delay(1000);
}





void loop() {
  perform_main->tic();
	
  // Read Data
  int gps_health = gps->read_all();
  
  perform_imu->tic();
  int imu_health = imu->Read();
  perform_imu->toc();

  perform_pwm->tic();
  duty_us = duty_us + 5;
  if(duty_us > 210)
    duty_us = 40;
  analogWrite(GPIO_D6, duty_us);
  perform_pwm->toc();

  // Menu Selection
  // int button = button_clicked;
  // button_clicked = false;
  if (button_clicked == true){
    button_clicked = false;
    menu_mode++;
    if(menu_mode > SHOW_END)
      menu_mode = SHOW_START;
  }
  rotary_right = false;
  rotary_left = false;
  button_s1 = false;
  button_s2 = false;

  /************************************/
  // Display
  /************************************/
  sprintf(monitor->msg[1], "C  %6d/A  %6d", perform_main->getdt(), perform_main->getdt() + perform_rest->getdt());
  switch(menu_mode){
    case SHOW_GPS:
      sprintf(monitor->msg[0], "T%5d   GPS %6d", cnt++, gps->perform_read.getdt());
      sprintf(monitor->msg[2], "Dsp%6d/Gps%6d", monitor->perform_print.getdt(), gps->perform_read.getdt());
      sprintf(monitor->msg[3], "Cnt %4d / Button %d", button_counter, button_clicked);
      sprintf(monitor->msg[4], "Date: %02d/%02d/20%02d",     gps->gps_device->day, gps->gps_device->month, gps->gps_device->year);
      sprintf(monitor->msg[5], "Time: %02d:%02d:%02d.%03d",  gps->gps_device->hour, gps->gps_device->minute, gps->gps_device->seconds, gps->gps_device->milliseconds);
      sprintf(monitor->msg[6], "Fix %d Qual %d Sat %2d",      (int)gps->gps_device->fix, (int)gps->gps_device->fixquality, (int)gps->gps_device->satellites);
      sprintf(monitor->msg[7], "%c%9.6f%c%10.6f",            gps->gps_device->lat, gps->gps_device->latitudeDegrees,gps->gps_device->lon, gps->gps_device->longitudeDegrees);
    break;
    case SHOW_IMU:
      sprintf(monitor->msg[0], "T%5d   IMU %6d", cnt++, perform_imu->getdt());
      sprintf(monitor->msg[2],"Ax %10.5f", imu->accel_x_mps2());
      sprintf(monitor->msg[3],"Ay %10.5f", imu->accel_y_mps2());
      sprintf(monitor->msg[4],"Az %10.5f", imu->accel_z_mps2());
      sprintf(monitor->msg[5],"Wx %10.5f", imu->gyro_x_radps());
      sprintf(monitor->msg[6],"Wy %10.5f", imu->gyro_y_radps());
      sprintf(monitor->msg[7],"Wz %10.5f", imu->gyro_z_radps());
    break;
    case SHOW_EST:{
      sprintf(monitor->msg[0], "T%5d   EST %6d", cnt++, perform_est->getdt());
      perform_est->tic();
      float ax = imu->accel_x_mps2();
      float ay = imu->accel_y_mps2();
      float az = imu->accel_z_mps2();
      float wx = imu->gyro_x_radps();
      float wy = imu->gyro_y_radps();
      float wz = imu->gyro_z_radps();
      float grav = sqrt(ax*ax + ay*ay + az*az);
      float phi_acc   = atan2(ay, -az);
      float theta_acc = asin(ax/grav);
      float phi_est   = Phi_cf->update(phi_acc, wx, dt);
      float theta_est = Theta_cf->update(theta_acc, wy, dt);
      perform_est->toc();
      sprintf(monitor->msg[2],"Ax %10.5f", imu->accel_x_mps2());
      sprintf(monitor->msg[3],"Ay %10.5f", imu->accel_y_mps2());
      sprintf(monitor->msg[4],"Az %10.5f", imu->accel_z_mps2());
      sprintf(monitor->msg[5], "Grav %10.7f", grav);
      sprintf(monitor->msg[6], "Ra%7.3f /Pa%7.3f", phi_acc * 57.296f, theta_acc * 57.296f);
      sprintf(monitor->msg[7], "Re%7.3f /Pe%7.3f", phi_est * 57.296f, theta_est * 57.296f);
    break;
    }
    case SHOW_PERFORM:
      sprintf(monitor->msg[0], "T%5d   PERFORM", cnt++);
      sprintf(monitor->msg[2], "MAIN %6d", perform_main->getdt());
      sprintf(monitor->msg[2], "DISP %6d", monitor->perform_print.getdt());
      sprintf(monitor->msg[3], "GPS  %6d", gps->perform_read.getdt());
      sprintf(monitor->msg[4], "IMU  %6d", perform_imu->getdt());
      sprintf(monitor->msg[5], "PWM  %6d", perform_pwm->getdt());
      sprintf(monitor->msg[6]," ");
      sprintf(monitor->msg[7]," ");
    break;
  }
  monitor->print();

	// Put in a slight delay to help debounce the reading
  perform_main->toc();
  
  perform_rest->tic();
  while(1){
    now_loop_time = micros();
    // Clock Overflow Case, each time after about 70 mins
    if(now_loop_time < last_loop_time){
      last_loop_time = 0;
      break;
    }
    // Normal Case
    else{
      delta_loop_time = now_loop_time - last_loop_time;
      if(delta_loop_time > size_loop_time){
        last_loop_time = last_loop_time + size_loop_time;
        break;
      }
    }
  }
  perform_rest->toc();
  
}


/************************************/
// Interrupt Service Routines
/************************************/
IRAM_ATTR void isr_rotary_s1(){
  if(rotary_last_press_s1 - millis() < 100)
    button_s1 = false;
  else{
    button_s1 = true;
    if(digitalRead(BUTTON_S2) == true){
      button_counter++;
      rotary_right=true;
    }
    else{
      button_counter--;
      rotary_right=false;
    }
  }
  rotary_last_press_s1 = millis();
}

// IRAM_ATTR void isr_rotary_s2(){
//   if(rotary_last_press_s2 - millis() < 100)
//     button_s2 = false;
//   else{
//     button_s2 = true;
//     if(digitalRead(BUTTON_S1) == true){
//       button_counter--;
//       rotary_left=true;
//     }
//   }
//   rotary_last_press_s2 = millis();
// }

IRAM_ATTR void isr_button_click(){
  if(button_last_press - millis() < 1000)
    button_clicked = false;
  else
    button_clicked = true;
  button_last_press = millis();
}