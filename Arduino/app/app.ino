/************************************/
// SimpleGoTo - NodeMCU v3 (ESP8266)
//   OLED SSD1306, rotary encoder, ICM20948 IMU, Adafruit Ultimate GPS,
//   RA axis DC motor on an H-bridge
/************************************/
#include "src/board_config.h"
#include "src/version.h"

#if defined(F_CPU) && F_CPU != 160000000L
#warning "Build at 160 MHz (Arduino IDE: Tools > CPU Frequency > 160 MHz, or scripts/build.sh)"
#endif
#include "src/display/oled.h"
#include "src/gps/gps.h"
#include "src/util/tictok_us.h"
#include "src/util/loop_timer.h"
#include "src/input/rotary.h"
#include "src/sensors/imu.h"
#include "src/sensors/battery.h"
#include "src/motor/ra_motor.h"
#include "src/app/core.h"
#include "src/app/boot_text.h"
#include "src/remote/remote_ui.h"
#include "src/config/eeprom_store.h"

/************************************/
// Devices
/************************************/
OLED*   monitor = new OLED();
GPS*    gps     = new GPS(&GPS_PORT);
Imu     imu;
RaMotor ra_motor(MOTOR_PWM, MOTOR_IN1, MOTOR_IN2);
Rotary  rotary(BUTTON_S1, BUTTON_S2, BUTTON_KEY);
BatteryMonitor battery(VBAT_FULL_SCALE_V, VBAT_FILTER_ALPHA);
unsigned long  battery_read_ms = 0;

/************************************/
// Everything that is not hardware: estimation, location/time, astro, UI, settings
// (src/app/core.h, the same code the UI simulator runs)
/************************************/
const float hz = MAIN_LOOP_HZ;
app::Core   core(hz, micros);
Frame       frame;
int         rotary_count_prev = 0;

/************************************/
// Settings in flash (docs/REQUIREMENTS.md DES-CFG)
/************************************/
config::EepromSettingsStore settings_store;

static_assert(sizeof(frame.lines) == sizeof(monitor->msg), "Frame and OLED buffers must match");

/************************************/
// Performance and simple scheduler
/************************************/
Tictok_us perform_main;
Tictok_us perform_rest;
Tictok_us perform_imu;
LoopTimer     loop_timer((unsigned long)(1000000.0f / hz));

// One boot message per line; when the screen is full the lines scroll up
// (line 0, the name and version, stays).
void bootLine(int& line, const char* text) {
  if (line >= DISP_LINE_COLUMN) {
    memmove(monitor->msg[1], monitor->msg[2], (DISP_LINE_COLUMN - 2) * DISP_LINE_ROW);
    line = DISP_LINE_COLUMN - 1;
  }
  snprintf(monitor->msg[line++], DISP_LINE_ROW, "%s", text);
  monitor->print();
}

// Shows a "starting ..." text on the next line without moving to it
void bootPending(int& line, const char* text) {
  bootLine(line, text);
  line--;                 // the next bootLine() overwrites it
}

void setup() {
  int line = 0;
#if !REMOTE_UI_ENABLE
  remote::begin();   // Wi-Fi off (REMOTE_UI_ENABLE 0)
#endif
  /************************************/
  // Display
  /************************************/
  monitor->setup_display();
  bootLine(line, boot::TITLE);
  char cpu_line[DISP_LINE_ROW];
  boot::displayLine(cpu_line, sizeof(cpu_line), ESP.getCpuFreqMHz());
  bootLine(line, cpu_line);

  /************************************/
  // IMU - ICM20948
  /************************************/
  bootPending(line, boot::IMU_PENDING);
  bool imu_on = imu.begin();
  bootLine(line, imu_on ? boot::IMU_ON : boot::IMU_FAILED);
  bootLine(line, imu.accelRangeText());
  bootLine(line, imu.gyroRangeText());
  bootLine(line, imu.magRateText());

  /************************************/
  // GPS
  /************************************/
  bootPending(line, boot::GPS_PENDING);
  bool gps_on = gps->setup_gps();
  bootLine(line, gps_on ? boot::GPS_ON : boot::GPS_TIMEOUT);

  /************************************/
  // Rotary encoder
  /************************************/
  bootPending(line, boot::BUTTONS_PENDING);
  rotary.begin();
  rotary_count_prev = rotary.count();
  bootLine(line, boot::BUTTONS_ON);

  /************************************/
  // Motor
  /************************************/
  ra_motor.begin();
  bootLine(line, boot::MOTOR_ON);

  /************************************/
  // Settings : calibration, RA duty, tracking offset (defaults if none or bad)
  /************************************/
  settings_store.begin();
  const config::LoadResult settings_result = core.begin(settings_store);   // also starts the estimator
  bootLine(line, config::loadText(settings_result));

#if REMOTE_UI_ENABLE
  /************************************/
  // Remote UI : Wi-Fi AP + web page + WebSocket
  /************************************/
  remote::begin();
  bootLine(line, boot::WIFI_AP);
#endif

  loop_timer.start(micros());
}

// Sensor values of this frame
void readInputs(app::FrameInput& in) {
  const Adafruit_GPS* g = gps->gps_device;
  in.now_ms           = millis();
  in.gps.year         = g->year;
  in.gps.month        = g->month;
  in.gps.day          = g->day;
  in.gps.hour         = g->hour;
  in.gps.minute       = g->minute;
  in.gps.seconds      = g->seconds;
  in.gps.milliseconds = g->milliseconds;
  in.gps.fixquality   = g->fixquality;
  in.gps.satellites   = g->satellites;
  in.gps.lat_hemi     = g->lat;
  in.gps.lon_hemi     = g->lon;
  in.gps.latitude     = ((double)g->latitude_fixed)  * 1E-7;
  in.gps.longitude    = ((double)g->longitude_fixed) * 1E-7;
  in.gps.altitude_m   = g->altitude;

  in.imu = imu.data();
  in.health.loop_hz = hz;
  in.health.loop_us = perform_rest.getdt() + perform_main.getdt();
  in.health.main_us = perform_main.getdt();
  in.health.disp_us = monitor->perform_print.getdt();
  in.health.gps_us  = gps->perform_read.getdt();
  in.health.imu_us  = perform_imu.getdt();   // est_us / astro_us: timed by core
  in.remote = remote::state();                 // status bar Wi-Fi mark
#if VBAT_SENSE_ENABLE
  if (!battery.valid() || in.now_ms - battery_read_ms >= VBAT_PERIOD_MS) {
    battery.update(analogRead(A0));
    battery_read_ms = in.now_ms;
  }
#endif
  in.power.valid = battery.valid();
  in.power.volts = battery.volts();
  // Phone time from the remote UI page (DES-LOC-05)
  in.phone_time = remote::takePhoneTime(in.phone_unix_s, in.phone_ms);
}

void loop() {
  perform_main.tic();
  app::FrameInput in;

  // Inputs
  int  count = rotary.count();
  in.delta = count - rotary_count_prev;
  rotary_count_prev = count;
  in.click = rotary.takeClick();
  // Phone (remote UI): dial steps add to the rotary, a click counts like the button
  int  remote_delta = 0;
  bool remote_click = false;
  remote::takeInput(remote_delta, remote_click);
  in.delta += remote_delta;
  in.click = in.click || remote_click;

  // Sensors
  gps->read_all();
  perform_imu.tic();
  imu.read();
  perform_imu.toc();
  readInputs(in);

  // Estimation, position/time, astro, UI (src/app/core.h)
  core.step(in, frame);

  // Display
  monitor->setPower(core.displayOn());   // Sleep: OLED off, phone still mirrors
  if (core.displayOn()) {
    memcpy(monitor->msg, frame.lines, sizeof(frame.lines));
    monitor->print();
  }
  remote::publish(frame);   // phone mirror (only when the lines changed)

  // Save / Reset from the UI, then the outputs (tracking overrides the manual RRcmd;
  // calibration / Polar Align stop the motor)
  ra_motor.setSpeed(core.finish(settings_store));

  perform_main.toc();

  /************************************/
  // Simple scheduler : wait for the next period
  /************************************/
  perform_rest.tic();
  while (!loop_timer.due(micros())) {
    remote::poll();   // HTTP / WebSocket (empty when REMOTE_UI_ENABLE is 0)
    yield();
  }
  perform_rest.toc();
}
