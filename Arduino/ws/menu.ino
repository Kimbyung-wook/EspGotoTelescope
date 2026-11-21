/************************************/
// Include Headers
/************************************/
#include "oled.h"
#include "gps.h"
#include "tictok_us.h"
#include "mpu6500.h" // 6.0.3
#include "esp_12f.h"
#define min(x, y) (x) < (y) ? (x) : (y)// -> x, y중에 작은값 반환
#define max(x, y) (x) > (y) ? (x) : (y)// -> x, y중에 큰값 반환

void setRotaryRight();
void setRotaryLeft();
void setRotaryNeutral();
void updateRotary();
void updateMenuCursor();


/************************************/
// Define Devices
/************************************/
#define GPS_PORT Serial
// #define DEBUG_PORT Serial1

/************************************/
// Display Settings
/************************************/
OLED* monitor           = new OLED();
GPS* gps                = new GPS(&GPS_PORT);
bfs::Mpu6500* imu       = new bfs::Mpu6500();

Tictok_us* perform_main = new Tictok_us();
Tictok_us* perform_rest = new Tictok_us();
Tictok_us* perform_imu  = new Tictok_us();
// Simple Scheduler
float hz = 10.0f;
unsigned long last_loop_time  = micros();
unsigned long now_loop_time   = micros();
unsigned long size_loop_time  = 1000 * (1000 / hz);
unsigned long delta_loop_time = 0;

/************************************/
// Rotary Button 
/************************************/
// Rotary Encoder Inputs
#define BUTTON_KEY  GPIO_D4
#define BUTTON_S1   GPIO_D6
#define BUTTON_S2   GPIO_D7
#define PWM1        GPIO_D5
#define PWM2        GPIO_D8
#define PWM3        GPIO_D3

volatile int rotary_pulse_s1 = 0;
volatile int rotary_pulse_s2 = 0;
volatile unsigned long rotary_last_pulse_s1 = 0;
volatile unsigned long rotary_last_pulse_s2 = 0;
volatile int rotary_state = 0;
volatile int rotary_state_prev = 0;
volatile int rotary_state_s1 = 0;
volatile int rotary_state_s2 = 0;
volatile int rotary_counter = 0;
volatile int rotary_counter_prev = 0;
volatile int rotary_counter_interval = 0;
volatile int rotary_right = false;
volatile int rotary_direction = 0;
volatile int rotary_left = false;
volatile int rotary_event = true;

volatile int button_counter = 0;
volatile int button_clicked = false;
volatile int did_button_clicked = false;
volatile unsigned long button_last_press = 0;

int rotation_speed_cnt_prev = 0;
float rotation_speed = 0.0f;
float rotation_speed_prev = 0.0f;

char main_menu_string[10][21]={
  "%c1RA Speed",
  "%c2Status - GPS",
  "%c3Status - Acc/Gyro",
  "%c4Status - Mag/Alt",
  "%c5Status - IMUCalib",
  "%c6Do IMU Calib.",
  "%c7Att.Estimator",
  "%c8Health",
  "%c9Axis for Moon",
  "%cSleep"
};

char cursor[6];
char cursor_icon = '>';
int cursor_pos = 0;
int menu_index = 0;
int menu_index_max = 0;
int menu_index_first = 0;

typedef enum Menu{
  main_menu = 0,
  
  ra_speed = 1,
  status_gps = 2,
  status_acc_gyro = 3,
  status_mag_alt = 4,
  status_imu_calib = 5,
  do_imu_calib = 6,
  att_est = 7,
  health = 8,
  moon_axis = 9,
  sleep = 10,

  ra_speed_select = 100,
}Menu;

void setup() {
  int system_on = true;
  /************************************/
  // Display Initialization
  /************************************/
  int monitor_on  = monitor->setup_display();
  system_on = system_on & monitor_on;
  sprintf(monitor->msg[2], "1. Display is ON");
  monitor->print();

  /************************************/
  // IMU Setup
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

  /************************************/
  // GPS Setup
  /************************************/
  sprintf(monitor->msg[4], "3. Intialize GPS");
  monitor->print();
  int gps_on      = gps->setup_gps();
  system_on = system_on & gps_on;
  if(gps_on == true)    sprintf(monitor->msg[4], "3. GPS is ON");
  else                  sprintf(monitor->msg[4], ">> GPS is timeout");
  monitor->print();
  /************************************/
  // Rotary Button Setup
  /************************************/
	// Set encoder pins as inputs
  sprintf(monitor->msg[5], "4. Intialize Buttons");
  monitor->print();
	pinMode(BUTTON_S1,  INPUT);
	pinMode(BUTTON_S2,  INPUT);
	pinMode(BUTTON_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S1),  isr_rotary_s1,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_S2),  isr_rotary_s2,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_KEY), isr_button_click,  RISING);
  sprintf(monitor->msg[5], "4. Buttons are ON");
  monitor->print();
  /************************************/
  // PWM
  /************************************/
  pinMode(PWM2, OUTPUT);
  analogWrite(PWM2, 120);
  sprintf(monitor->msg[6], "5. PWM is ON");
  monitor->print();
}

void loop() {	
  perform_main->tic();
  gps->read_all();
  perform_imu->tic();
  int imu_health = imu->Read();
  perform_imu->toc();


  float ax = imu->accel_x_mps2();
  float ay = imu->accel_y_mps2();
  float az = imu->accel_z_mps2();
  float grav = sqrt(ax*ax + ay*ay + az*az);
  float phi_acc   = atan2(ay, -az) * 57.296f;
  float theta_acc = asin(ax/grav)  * 57.296f;
  double latitude  = ((double)gps->gps_device->latitude_fixed)*1E-7;
  double longitude = ((double)gps->gps_device->longitude_fixed)*1E-7;

  /************************************/
  // Display
  /************************************/
  // Top Window
  sprintf(monitor->msg[0], "T %2d:%2d:%2d.%1d V %5.2f",
          gps->gps_device->hour, gps->gps_device->minute,
          gps->gps_device->seconds, gps->gps_device->milliseconds/100, 12.60f);
  // sprintf(monitor->msg[1], "Button %2d Rotary %2d",
  //         button_counter, rotary_counter);

  // Sub Window
  switch(menu_index)
  {
    case Menu::main_menu:
    {
      menu_index_max = 10;
      updateMenuCursor();
      menu_index_first = max(0, rotary_counter_interval - cursor_pos - 1);
      
      sprintf(monitor->msg[1], "Main Menu");
      sprintf(monitor->msg[2], main_menu_string[0 + menu_index_first], cursor[0]);
      sprintf(monitor->msg[3], main_menu_string[1 + menu_index_first], cursor[1]);
      sprintf(monitor->msg[4], main_menu_string[2 + menu_index_first], cursor[2]);
      sprintf(monitor->msg[5], main_menu_string[3 + menu_index_first], cursor[3]);
      sprintf(monitor->msg[6], main_menu_string[4 + menu_index_first], cursor[4]);
      sprintf(monitor->msg[7], main_menu_string[5 + menu_index_first], cursor[5]);
      
      if(did_button_clicked == true)
      {
        menu_index = menu_index_first + cursor_pos + 1;
        cursor_pos = 0;
      }
    }
    break;
    case Menu::ra_speed:
    {
      menu_index_max = 2;
      updateMenuCursor();
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], "%cRRcmd %9.4fDEG", cursor[0], rotation_speed);
      sprintf(monitor->msg[3], "%cRRate %9.4fDEG", cursor[1], imu->gyro_x_radps());
      sprintf(monitor->msg[4], "%cQuit"           , cursor[2]);
      sprintf(monitor->msg[5], " ROL %9.4f  DEG",  phi_acc);
      sprintf(monitor->msg[6], " LAT %c%10.6fDEG", gps->gps_device->lat, latitude);
      sprintf(monitor->msg[7], " PIT %9.4f  DEG",  theta_acc);
      if(did_button_clicked == true)
      {
        if(cursor_pos == 0)
        {
          menu_index = Menu::ra_speed_select;
          rotary_counter_prev = rotary_counter;
        }
        else if(cursor_pos == menu_index_max)
        {
          rotary_counter_prev = rotary_counter - menu_index + 1;
          menu_index = Menu::main_menu;
        }
      }
    }
    break;
    case Menu::status_gps:
    {
      menu_index_max = 0;
      char fixqual[5];
      if(gps->gps_device->fixquality == 0)
        sprintf(fixqual, "NoFix");
      else if (gps->gps_device->fixquality == 1)
        sprintf(fixqual, "GPS");
      else if (gps->gps_device->fixquality == 2)
        sprintf(fixqual, "DGPS");

      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], "Date: 20%02d/%02d/%02d",      gps->gps_device->year, gps->gps_device->month, gps->gps_device->day);
      sprintf(monitor->msg[3], "Time:   %02d:%02d:%02d.%03d", gps->gps_device->hour, gps->gps_device->minute, gps->gps_device->seconds, gps->gps_device->milliseconds);
      sprintf(monitor->msg[4], "%5s Sat %2d",                 fixqual, (int)gps->gps_device->satellites);
      sprintf(monitor->msg[5], " ");
      sprintf(monitor->msg[6], " %c %12.8f DEG",              gps->gps_device->lat, latitude);
      sprintf(monitor->msg[7], " %c %12.8f DEG",              gps->gps_device->lon, longitude);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::status_acc_gyro:
    {
      menu_index_max = 0;
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2],"Ax    %9.4f mpss", imu->accel_x_mps2());
      sprintf(monitor->msg[3],"Ay    %9.4f mpss", imu->accel_y_mps2());
      sprintf(monitor->msg[4],"Az    %9.4f mpss", imu->accel_z_mps2());
      sprintf(monitor->msg[5],"Wx    %9.4f dps",  imu->gyro_x_radps());
      sprintf(monitor->msg[6],"Wy    %9.4f dps",  imu->gyro_y_radps());
      sprintf(monitor->msg[7],"Wz    %9.4f dps",  imu->gyro_z_radps());
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::status_mag_alt:
    {
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], "Mx   %9.4f uT");
      sprintf(monitor->msg[3], "My   %9.4f uT");
      sprintf(monitor->msg[4], "Mz   %9.4f uT");
      sprintf(monitor->msg[5], "Head %9.4f DEG");
      sprintf(monitor->msg[6], " ");
      sprintf(monitor->msg[7], " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::status_imu_calib:
    {
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], "bAx  %9.4f mpss", 0.0f);
      sprintf(monitor->msg[3], "bAy  %9.4f mpss", 0.0f);
      sprintf(monitor->msg[4], "bAz  %9.4f mpss", 0.0f);
      sprintf(monitor->msg[5], "bWx  %9.4f dps",  0.0f);
      sprintf(monitor->msg[6], "bWy  %9.4f dps",  0.0f);
      sprintf(monitor->msg[7], "bWz  %9.4f dps",  0.0f);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::do_imu_calib:
    {
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], " ");
      sprintf(monitor->msg[3], " ");
      sprintf(monitor->msg[4], " ");
      sprintf(monitor->msg[5], " ");
      sprintf(monitor->msg[6], " ");
      sprintf(monitor->msg[7], " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::att_est:
    {
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], " ");
      sprintf(monitor->msg[3], " ");
      sprintf(monitor->msg[4], " ");
      sprintf(monitor->msg[5], " ");
      sprintf(monitor->msg[6], " ");
      sprintf(monitor->msg[7], " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::health:
    {
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], "Loop %6d/%5.1fHz", perform_rest->getdt() + perform_main->getdt(), hz);
      sprintf(monitor->msg[3], "MAIN %6d", perform_main->getdt());
      sprintf(monitor->msg[4], "DISP %6d", monitor->perform_print.getdt());
      sprintf(monitor->msg[5], "GPS  %6d", gps->perform_read.getdt());
      sprintf(monitor->msg[6], "IMU  %6d", perform_imu->getdt());
      sprintf(monitor->msg[7], "EST  %6d", 0);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::moon_axis:
    {
      updateMenuCursor();
      sprintf(monitor->msg[1], main_menu_string[menu_index-1], '*');
      sprintf(monitor->msg[2], "ROLL  %9.4f DEG", phi_acc);
      sprintf(monitor->msg[3], "PITCH %9.4f DEG", theta_acc);
      sprintf(monitor->msg[4], "YAW   %9.4f DEG", 120.0012f);
      sprintf(monitor->msg[5], "-- Moon --------");
      sprintf(monitor->msg[6], "R.A.  %9.4f DEG",  37.0013f);
      sprintf(monitor->msg[7], "DEC.  %9.4f DEG",-120.0014f);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::sleep:
    {
      sprintf(monitor->msg[1], " ");
      sprintf(monitor->msg[2], " ");
      sprintf(monitor->msg[3], " ");
      sprintf(monitor->msg[4], " ");
      sprintf(monitor->msg[5], " ");
      sprintf(monitor->msg[6], " ");
      sprintf(monitor->msg[7], " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::ra_speed_select:
    {
      rotation_speed = rotation_speed_prev + 0.1f * (rotary_counter - rotation_speed_cnt_prev);
      analogWrite(PWM2, (int)(rotation_speed * 10.0f));
      sprintf(monitor->msg[1], main_menu_string[1], '*');
      sprintf(monitor->msg[2], "%cRRcmd %9.4fDEG", cursor_icon, rotation_speed);
      sprintf(monitor->msg[3], " RRate %9.4fDEG",  imu->gyro_x_radps());
      sprintf(monitor->msg[4], " Exit");
      sprintf(monitor->msg[5], " ROL %9.4f  DEG",  phi_acc);
      sprintf(monitor->msg[6], " LAT %c%10.6fDEG", gps->gps_device->lat, latitude);
      sprintf(monitor->msg[7], " PIT %9.4f  DEG",  theta_acc);
      if(did_button_clicked == true)
      {
        menu_index = Menu::ra_speed;
      }
      rotary_counter_prev = rotary_counter + 1;
      rotation_speed_cnt_prev = rotary_counter;
      rotation_speed_prev = rotation_speed;
    }
    break;
    default:
    {
      sprintf(monitor->msg[1], "ERROR");
      sprintf(monitor->msg[2], " ");
      sprintf(monitor->msg[3], " ");
      sprintf(monitor->msg[4], " ");
      sprintf(monitor->msg[5], " ");
      sprintf(monitor->msg[6], " ");
      sprintf(monitor->msg[7], " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter + 1;
        menu_index = Menu::main_menu;
      }
    }
  }
  did_button_clicked = false;
  if(cursor_icon == '>')
    cursor_icon = '-';
  else
    cursor_icon = '>';
  monitor->print();
  
  perform_main->toc();
  /************************************/
  // Simple Scheduler
  /************************************/
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
//3201 3201
// 0 -> 4 (2(0+1)+2) -> 10 (2*(4+1)+0) -> 23 (2*(10+1)+1)
void setRotaryRight(){
  rotary_counter++;
  rotary_direction = 1; // left
  rotary_event = true;
}
// 3102 3102
// 0 -> 3 (2*(0+1)+1) -> 8 (2*(3+1)+0) -> 20 (2*(8+1)+2)
void setRotaryLeft(){
  rotary_counter--;
  rotary_direction = -1; // right
  rotary_event = true;
}
void setRotaryNeutral(){
  rotary_event = false;
  rotary_direction = 0; // neutral
}
void updateRotary(){
  rotary_state_s1 = digitalRead(BUTTON_S1);
  rotary_state_s2 = digitalRead(BUTTON_S2);
  rotary_state = 2 * rotary_state_s1 + rotary_state_s2;
  if(rotary_state_prev == 3 && rotary_state == 2){
    setRotaryRight();
  }
  if(rotary_state_prev == 0 && rotary_state == 1){
    setRotaryRight();
  }

  if(rotary_state_prev == 1 && rotary_state == 0){
    setRotaryLeft();
  }
  if(rotary_state_prev == 2 && rotary_state == 3){
    setRotaryLeft();
  }
  rotary_state_prev = rotary_state;
}

/************************************/
// Interrupt Service Routines
/************************************/
// CW  (+, R) : S1 true,   S2 Rising
// CCW (-, L) : S1 Rising, S2 true
IRAM_ATTR void isr_rotary_s1(){
  // prevent being clicked over twice at once
  if(rotary_last_pulse_s1 - millis() < 30)
    rotary_pulse_s1 = false;
  else{
    rotary_pulse_s1 = true;
  }
  rotary_last_pulse_s1 = millis();

  // processing rotary action
  if(rotary_pulse_s1 == true){
    rotary_pulse_s1 = false;
    updateRotary();
  }
}

IRAM_ATTR void isr_rotary_s2(){
  // prevent being clicked over twice at once
  if(rotary_last_pulse_s2 - millis() < 30)
    rotary_pulse_s2 = false;
  else{
    rotary_pulse_s2 = true;
  }
  rotary_last_pulse_s2 = millis();

  // processing rotary action
  if(rotary_pulse_s2 == true){
    rotary_pulse_s2 = false;
    updateRotary();
  }
}

// Button Interrupt Routine
IRAM_ATTR void isr_button_click(){
  // prevent being clicked over twice at once
  if(button_last_press - millis() < 500)
    button_clicked = false;
  else
    button_clicked = true;
  button_last_press = millis();

  // processing button action
  // throw main loop the fact being clicked
  if(button_clicked == true){
    button_clicked = false;
    did_button_clicked = true;
    button_counter++;
  }
}
void updateMenuCursor()
{
  rotary_counter_interval = rotary_counter - rotary_counter_prev;
  if(rotary_counter_interval < 0)
    rotary_counter_prev = rotary_counter;
  else if(rotary_counter_interval > menu_index_max - 1)
    rotary_counter_prev = rotary_counter - menu_index_max;
  rotary_counter_interval = rotary_counter - rotary_counter_prev;
  cursor_pos = min(max(0, rotary_counter_interval), 5);

  for (int i = 0; i < 6; i++)
  {
    if (i == cursor_pos)
      cursor[i] = cursor_icon;
    else
      cursor[i] = ' ';
  }
}