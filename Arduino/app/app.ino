/************************************/
// Include Headers
/************************************/
#include "src/board_config.h"
#include "src/display/oled.h"
#include "src/gps/gps.h"
#include "src/util/tictok_us.h"
#include "src/input/rotary.h"
#include "src/sensors/imu.h"
#include "src/motor/ra_motor.h"
#include "src/gps/gps_link.h"

#define min(x, y) (x) < (y) ? (x) : (y)// -> x, y중에 작은값 반환
#define max(x, y) (x) > (y) ? (x) : (y)// -> x, y중에 큰값 반환

void updateMenuCursor();


/************************************/
// Define Devices
/************************************/

/************************************/
// Display Settings
/************************************/
OLED* monitor           = new OLED();
GPS* gps                = new GPS(&GPS_PORT);
Imu imu;
RaMotor ra_motor(MOTOR_PWM, MOTOR_IN1, MOTOR_IN2);

Tictok_us* perform_main = new Tictok_us();
Tictok_us* perform_rest = new Tictok_us();
Tictok_us* perform_imu  = new Tictok_us();
// Simple Scheduler
float hz = MAIN_LOOP_HZ;
unsigned long last_loop_time  = micros();
unsigned long now_loop_time   = micros();
unsigned long size_loop_time  = 1000 * (1000 / hz);
unsigned long delta_loop_time = 0;

/************************************/
// Rotary Button 
/************************************/
// Pins are defined in src/board_config.h

Rotary rotary(BUTTON_S1, BUTTON_S2, BUTTON_KEY);
int  rotary_counter = 0;           // snapshot of rotary.count() for this frame
int  rotary_counter_prev = 0;
int  rotary_counter_interval = 0;
bool did_button_clicked = false;   // snapshot of rotary.takeClick() for this frame

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


float ax, ay, az;
float wx, wy, wz;
float mx, my, mz;
float bax, bay, baz;
float bwx, bwy, bwz;
float temperature;
float grav;
float phi_acc;
float theta_acc;
unsigned long last_calib_time_since = micros();


typedef enum Menu{
  main_menu = 0,
  
  ra_speed = 1,
  status_gps = 2,
  status_acc_gyro = 3,
  status_mag_temp = 4,
  status_imu_calib = 5,
  do_imu_calib = 6,
  att_est = 7,
  health = 8,
  moon_axis = 9,
  sleep = 10,

  ra_speed_select = 100,
  calib_acc_bias = 101,
  calib_acc_scale = 102,
  calib_gyro_bias = 103,
  calib_gyro_scale = 104,
}Menu;

void setup() {
  int line = 0;
  int system_on = true;
  /************************************/
  // Display Initialization
  /************************************/
  int monitor_on  = monitor->setup_display();
  system_on = system_on & monitor_on;
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "1. Display is ON");
  monitor->print();

  /************************************/
  // IMU Setup - ICM 20948
  /************************************/
  snprintf(monitor->msg[line], DISP_LINE_ROW, "2. Intialize IMU");
  monitor->print();
  int imu_on = imu.begin();
  system_on = system_on & imu_on;
  if(imu_on == true)    snprintf(monitor->msg[line++], DISP_LINE_ROW, "2. IMU is ON");
  else                  snprintf(monitor->msg[line++], DISP_LINE_ROW, ">> IMU failed");
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "%s", imu.accelRangeText());
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "%s", imu.gyroRangeText());
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "%s", imu.magRateText());
  monitor->print();

  /************************************/
  // GPS Setup
  /************************************/
  snprintf(monitor->msg[line], DISP_LINE_ROW, "3. Intialize GPS");
  monitor->print();
  int gps_on      = gps->setup_gps();
  system_on = system_on & gps_on;
  if(gps_on == true)    snprintf(monitor->msg[line++], DISP_LINE_ROW, "3. GPS is ON");
  else                  snprintf(monitor->msg[line++], DISP_LINE_ROW, ">> GPS is timeout");
  monitor->print();
  /************************************/
  // Rotary Button Setup
  /************************************/
  snprintf(monitor->msg[line], DISP_LINE_ROW, "4. Intialize Buttons");
  monitor->print();
  rotary.begin();
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "4. Buttons are ON");
  monitor->print();
  /************************************/
  // PWM
  /************************************/
  ra_motor.begin();
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "5. GPIO/PWM is ON");
  monitor->print();
}

void loop() {	
  perform_main->tic();
  rotary_counter     = rotary.count();
  did_button_clicked = rotary.takeClick();
  gps->read_all();
  double latitude  = ((double)gps->gps_device->latitude_fixed)*1E-7;
  double longitude = ((double)gps->gps_device->longitude_fixed)*1E-7;

  perform_imu->tic();
  imu.read();
  const ImuData& d = imu.data();
  ax = d.ax;  ay = d.ay;  az = d.az;
  wx = d.wx;  wy = d.wy;  wz = d.wz;
  mx = d.mx;  my = d.my;  mz = d.mz;
  temperature = d.temperature;
  grav      = accelNorm(d);
  phi_acc   = rollFromAccelDeg(d);
  theta_acc = pitchFromAccelDeg(d);
  perform_imu->toc();



  /************************************/
  // Display
  /************************************/
  // Top Window
  snprintf(monitor->msg[0], DISP_LINE_ROW, "T %2d:%2d:%2d.%1d V %5.2f",
          gps->gps_device->hour, gps->gps_device->minute,
          gps->gps_device->seconds, gps->gps_device->milliseconds/100, 12.60f);
  // snprintf(monitor->msg[1], DISP_LINE_ROW, "Button %2d Rotary %2d",
  //         rotary.clickCount(), rotary_counter);

  // Sub Window
  switch(menu_index)
  {
    case Menu::main_menu:
    {
      menu_index_max = 10-1; // 10 items, index 0..9
      updateMenuCursor();
      menu_index_first = max(0, rotary_counter_interval - cursor_pos);
      
      snprintf(monitor->msg[1], DISP_LINE_ROW, "Main Menu");
      snprintf(monitor->msg[2], DISP_LINE_ROW, main_menu_string[0 + menu_index_first], cursor[0]);
      snprintf(monitor->msg[3], DISP_LINE_ROW, main_menu_string[1 + menu_index_first], cursor[1]);
      snprintf(monitor->msg[4], DISP_LINE_ROW, main_menu_string[2 + menu_index_first], cursor[2]);
      snprintf(monitor->msg[5], DISP_LINE_ROW, main_menu_string[3 + menu_index_first], cursor[3]);
      snprintf(monitor->msg[6], DISP_LINE_ROW, main_menu_string[4 + menu_index_first], cursor[4]);
      snprintf(monitor->msg[7], DISP_LINE_ROW, main_menu_string[5 + menu_index_first], cursor[5]);
      
      if(did_button_clicked == true)
      {
        menu_index = menu_index_first + cursor_pos + 1;
        cursor_pos = 0;
        rotary_counter_prev = rotary_counter; // sub menu starts at the first line
      }
    }
    break;
    case Menu::ra_speed:
    {
      menu_index_max = 3-1;
      updateMenuCursor();
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "%cRRcmd %10.5fDEG", cursor[0], rotation_speed);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "%cRRate %10.5fDPS", cursor[1], wx);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "%cQuit"           , cursor[2]);
      snprintf(monitor->msg[5], DISP_LINE_ROW, " ROL %10.5f  DEG",  phi_acc);
      snprintf(monitor->msg[6], DISP_LINE_ROW, " LAT %c%10.6fDEG", gps->gps_device->lat, latitude);
      snprintf(monitor->msg[7], DISP_LINE_ROW, " PIT %10.5f  DEG",  theta_acc);
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
      char fixqual[6];
      gpsFixText(gps->gps_device->fixquality, fixqual, sizeof(fixqual));

      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "Date: 20%02d/%02d/%02d",      gps->gps_device->year, gps->gps_device->month, gps->gps_device->day);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "Time:   %02d:%02d:%02d.%03d", gps->gps_device->hour, gps->gps_device->minute, gps->gps_device->seconds, gps->gps_device->milliseconds);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "%5s Sat %2d",                 fixqual, (int)gps->gps_device->satellites);
      snprintf(monitor->msg[5], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[6], DISP_LINE_ROW, " %c %12.8f DEG",              gps->gps_device->lat, latitude);
      snprintf(monitor->msg[7], DISP_LINE_ROW, " %c %12.8f DEG",              gps->gps_device->lon, longitude);
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
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "Ax    %10.5f mpss", ax);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "Ay    %10.5f mpss", ay);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "Az    %10.5f mpss", az);
      snprintf(monitor->msg[5], DISP_LINE_ROW, "Wx    %10.5f dps",  wx);
      snprintf(monitor->msg[6], DISP_LINE_ROW, "Wy    %10.5f dps",  wy);
      snprintf(monitor->msg[7], DISP_LINE_ROW, "Wz    %10.5f dps",  wz);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::status_mag_temp:
    {
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "Mx   %10.5f uT", mx);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "My   %10.5f uT", my);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "Mz   %10.5f uT", mz);
      snprintf(monitor->msg[5], DISP_LINE_ROW, "Temp %10.5f 'c ", temperature);
      snprintf(monitor->msg[6], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[7], DISP_LINE_ROW, " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::status_imu_calib:
    {
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "bAx  %10.5f mpss", 0.0f);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "bAy  %10.5f mpss", 0.0f);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "bAz  %10.5f mpss", 0.0f);
      snprintf(monitor->msg[5], DISP_LINE_ROW, "bWx  %10.5f dps",  0.0f);
      snprintf(monitor->msg[6], DISP_LINE_ROW, "bWy  %10.5f dps",  0.0f);
      snprintf(monitor->msg[7], DISP_LINE_ROW, "bWz  %10.5f dps",  0.0f);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::do_imu_calib:
    {
      menu_index_max = 6-1;
      updateMenuCursor();
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "%cAcc. Bias Calib", cursor[0]);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "%cAcc. Scale Calib", cursor[1]);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "%cGyro Bias Calib", cursor[2]);
      snprintf(monitor->msg[5], DISP_LINE_ROW, "%cGyro Scale Calib", cursor[3]);
      snprintf(monitor->msg[6], DISP_LINE_ROW, "%c", cursor[4]);
      snprintf(monitor->msg[7], DISP_LINE_ROW, "%cQuit", cursor[5]);
      if(did_button_clicked == true)
      {
        switch (cursor_pos)
        {
          case 0: // Acc Bias Calib
          break;
          case 1: // Acc Scale Calib
          break;
          case 2: // Gyro Bias Calib
            menu_index = Menu::calib_gyro_bias;
            last_calib_time_since = millis();
          break;
          case 3: // Gyro Scale Calib
          break;
          case 5:
            rotary_counter_prev = rotary_counter - Menu::do_imu_calib + 1;
            menu_index = Menu::main_menu;
          break;
        }
      }
    }
    break;
    case Menu::att_est:
    {
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "ROLL   %10.5f DEG", phi_acc);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "PITCH  %10.5f DEG", theta_acc);
      snprintf(monitor->msg[4], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[5], DISP_LINE_ROW, "Wx     %10.5f DPS", wx);
      snprintf(monitor->msg[6], DISP_LINE_ROW, "Wy     %10.5f DPS", wy);
      snprintf(monitor->msg[7], DISP_LINE_ROW, " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::health:
    {
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "Loop %6d/%5.1fHz", perform_rest->getdt() + perform_main->getdt(), hz);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "MAIN %6d", perform_main->getdt());
      snprintf(monitor->msg[4], DISP_LINE_ROW, "DISP %6d", monitor->perform_print.getdt());
      snprintf(monitor->msg[5], DISP_LINE_ROW, "GPS  %6d", gps->perform_read.getdt());
      snprintf(monitor->msg[6], DISP_LINE_ROW, "IMU  %6d", perform_imu->getdt());
      snprintf(monitor->msg[7], DISP_LINE_ROW, "EST  %6d", 0);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::moon_axis:
    {
      updateMenuCursor();
      snprintf(monitor->msg[1], DISP_LINE_ROW, main_menu_string[menu_index-1], '*');
      snprintf(monitor->msg[2], DISP_LINE_ROW, "ROLL  %10.5f DEG", phi_acc);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "PITCH %10.5f DEG", theta_acc);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "YAW   %10.5f DEG", 120.0012f);
      snprintf(monitor->msg[5], DISP_LINE_ROW, "-- Moon --------");
      snprintf(monitor->msg[6], DISP_LINE_ROW, "R.A.  %10.5f DEG",  37.0013f);
      snprintf(monitor->msg[7], DISP_LINE_ROW, "DEC.  %10.5f DEG",-120.0014f);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - menu_index + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::sleep:
    {
      snprintf(monitor->msg[1], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[2], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[3], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[4], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[5], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[6], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[7], DISP_LINE_ROW, " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter + 1;
        menu_index = Menu::main_menu;
      }
    }
    break;
    case Menu::ra_speed_select:
    {
      rotation_speed = rotation_speed_prev + 1.0f * (rotary_counter - rotation_speed_cnt_prev);
      // float spd = rotation_speed * 10.0f;
      ra_motor.setSpeed(rotation_speed);
      snprintf(monitor->msg[1], DISP_LINE_ROW, "*Set RA Speed");
      snprintf(monitor->msg[2], DISP_LINE_ROW, "%cRRcmd %10.5fDEG", cursor_icon, rotation_speed);
      snprintf(monitor->msg[3], DISP_LINE_ROW, " RRate %10.5fDPS",  wx);
      snprintf(monitor->msg[4], DISP_LINE_ROW, " Exit");
      snprintf(monitor->msg[5], DISP_LINE_ROW, " ROL %10.5f  DEG",  phi_acc);
      snprintf(monitor->msg[6], DISP_LINE_ROW, " LAT %c%10.6fDEG", gps->gps_device->lat, latitude);
      snprintf(monitor->msg[7], DISP_LINE_ROW, " PIT %10.5f  DEG",  theta_acc);
      if(did_button_clicked == true)
      {
        menu_index = Menu::ra_speed;
      }
      rotary_counter_prev = rotary_counter + 1;
      rotation_speed_cnt_prev = rotary_counter;
      rotation_speed_prev = rotation_speed;
    }
    break;
    
    case calib_acc_bias:
    {
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - 0;
        menu_index = Menu::do_imu_calib;
      }
    }
    break;
    case calib_acc_scale:
    {
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - 1;
        menu_index = Menu::do_imu_calib;
      }
    }
    break;
    case calib_gyro_bias:
    {
      float alpha = 0.01;
      if((millis() - last_calib_time_since) < 1000)
      {
        bwx = wx;
        bwy = wy;
        bwz = wz;
        snprintf(monitor->msg[1], DISP_LINE_ROW, "Calib Gyro Bias Ready");
      }
      else if((millis() - last_calib_time_since) < 6000)
      {
        alpha = 0.3;
        bwx = (1.0 - alpha) * bwx + alpha * wx;
        bwy = (1.0 - alpha) * bwy + alpha * wy;
        bwz = (1.0 - alpha) * bwz + alpha * wz;
        snprintf(monitor->msg[1], DISP_LINE_ROW, "Calib Gyro Bias ing.1");
      }
      else if((millis() - last_calib_time_since) < 16000)
      {
        alpha = 0.01;
        bwx = (1.0 - alpha) * bwx + alpha * wx;
        bwy = (1.0 - alpha) * bwy + alpha * wy;
        bwz = (1.0 - alpha) * bwz + alpha * wz;
        snprintf(monitor->msg[1], DISP_LINE_ROW, "Calib Gyro Bias ing.2");
      }
      else
      {
        snprintf(monitor->msg[1], DISP_LINE_ROW, "Calib Gyro Bias Done.");
      }
      snprintf(monitor->msg[2], DISP_LINE_ROW, "Wx %8.5f->%8.5f", wx, wx - bwx);
      snprintf(monitor->msg[3], DISP_LINE_ROW, "Wy %8.5f->%8.5f", wy, wy - bwy);
      snprintf(monitor->msg[4], DISP_LINE_ROW, "Wz %8.5f->%8.5f", wz, wz - bwz);
      snprintf(monitor->msg[5], DISP_LINE_ROW, "bWx   %10.5f dps",  bwx);
      snprintf(monitor->msg[6], DISP_LINE_ROW, "bWy   %10.5f dps",  bwy);
      snprintf(monitor->msg[7], DISP_LINE_ROW, "bWz   %10.5f dps",  bwz);
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - 2;
        menu_index = Menu::do_imu_calib;
      }
    }
    break;
    case calib_gyro_scale:
    {
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter - 3;
        menu_index = Menu::do_imu_calib;
      }
    }
    break;
    default:
    {
      snprintf(monitor->msg[1], DISP_LINE_ROW, "ERROR");
      snprintf(monitor->msg[2], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[3], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[4], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[5], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[6], DISP_LINE_ROW, " ");
      snprintf(monitor->msg[7], DISP_LINE_ROW, " ");
      if(did_button_clicked == true)
      {
        rotary_counter_prev = rotary_counter + 1;
        menu_index = Menu::main_menu;
      }
    }
  }
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