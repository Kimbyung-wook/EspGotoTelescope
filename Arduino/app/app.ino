/************************************/
// Automatic Telescope - NodeMCU v3 (ESP8266)
//   OLED SSD1306, rotary encoder, ICM20948 IMU, Adafruit Ultimate GPS,
//   RA axis DC motor on an H-bridge
/************************************/
#include "src/board_config.h"
#include "src/display/oled.h"
#include "src/gps/gps.h"
#include "src/util/tictok_us.h"
#include "src/input/rotary.h"
#include "src/sensors/imu.h"
#include "src/motor/ra_motor.h"
#include "src/ui/telescope_ui.h"

/************************************/
// Devices
/************************************/
OLED*   monitor = new OLED();
GPS*    gps     = new GPS(&GPS_PORT);
Imu     imu;
RaMotor ra_motor(MOTOR_PWM, MOTOR_IN1, MOTOR_IN2);
Rotary  rotary(BUTTON_S1, BUTTON_S2, BUTTON_KEY);

/************************************/
// UI
/************************************/
TelescopeUi ui;
UiContext   ui_ctx;
Frame       frame;
int         rotary_count_prev = 0;

static_assert(sizeof(frame.lines) == sizeof(monitor->msg), "Frame and OLED buffers must match");

/************************************/
// Performance and simple scheduler
/************************************/
Tictok_us perform_main;
Tictok_us perform_rest;
Tictok_us perform_imu;
const float   hz             = MAIN_LOOP_HZ;
unsigned long last_loop_time = micros();
unsigned long size_loop_time = 1000 * (1000 / hz);

void bootLine(int& line, const char* text) {
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "%s", text);
  monitor->print();
}

void setup() {
  int line = 0;
  /************************************/
  // Display
  /************************************/
  monitor->setup_display();
  bootLine(line, "1. Display is ON");

  /************************************/
  // IMU - ICM20948
  /************************************/
  snprintf(monitor->msg[line], DISP_LINE_ROW, "2. Intialize IMU");
  monitor->print();
  bool imu_on = imu.begin();
  bootLine(line, imu_on ? "2. IMU is ON" : ">> IMU failed");
  bootLine(line, imu.accelRangeText());
  bootLine(line, imu.gyroRangeText());
  bootLine(line, imu.magRateText());

  /************************************/
  // GPS
  /************************************/
  snprintf(monitor->msg[line], DISP_LINE_ROW, "3. Intialize GPS");
  monitor->print();
  bool gps_on = gps->setup_gps();
  bootLine(line, gps_on ? "3. GPS is ON" : ">> GPS is timeout");

  /************************************/
  // Rotary encoder
  /************************************/
  snprintf(monitor->msg[line], DISP_LINE_ROW, "4. Intialize Buttons");
  monitor->print();
  rotary.begin();
  rotary_count_prev = rotary.count();
  bootLine(line, "4. Buttons are ON");

  /************************************/
  // Motor
  /************************************/
  ra_motor.begin();
  bootLine(line, "5. GPIO/PWM is ON");
}

// Copy sensor values into the UI telemetry
void updateTelemetry(Telemetry& t) {
  const Adafruit_GPS* g = gps->gps_device;
  t.now_ms       = millis();
  t.year         = g->year;
  t.month        = g->month;
  t.day          = g->day;
  t.hour         = g->hour;
  t.minute       = g->minute;
  t.seconds      = g->seconds;
  t.milliseconds = g->milliseconds;
  t.fixquality   = g->fixquality;
  t.satellites   = g->satellites;
  t.lat_hemi     = g->lat;
  t.lon_hemi     = g->lon;
  t.latitude     = ((double)g->latitude_fixed)  * 1E-7;
  t.longitude    = ((double)g->longitude_fixed) * 1E-7;

  t.imu           = imu.data();
  t.roll_acc_deg  = rollFromAccelDeg(t.imu);
  t.pitch_acc_deg = pitchFromAccelDeg(t.imu);

  t.loop_hz = hz;
  t.loop_us = perform_rest.getdt() + perform_main.getdt();
  t.main_us = perform_main.getdt();
  t.disp_us = monitor->perform_print.getdt();
  t.gps_us  = gps->perform_read.getdt();
  t.imu_us  = perform_imu.getdt();
  t.est_us  = 0;
}

void loop() {
  perform_main.tic();

  // Inputs
  int  count = rotary.count();
  int  delta = count - rotary_count_prev;
  rotary_count_prev = count;
  bool click = rotary.takeClick();

  // Sensors
  gps->read_all();
  perform_imu.tic();
  imu.read();
  perform_imu.toc();
  updateTelemetry(ui_ctx.tel);

  // UI -> display
  frame.clear();
  ui.step(delta, click, ui_ctx, frame);
  memcpy(monitor->msg, frame.lines, sizeof(frame.lines));
  monitor->print();

  // Outputs
  ra_motor.setSpeed(ui_ctx.state.ra_speed_cmd);

  perform_main.toc();

  /************************************/
  // Simple scheduler : wait for the next period
  /************************************/
  perform_rest.tic();
  while (micros() - last_loop_time < size_loop_time) {
  }
  last_loop_time += size_loop_time;
  perform_rest.toc();
}
