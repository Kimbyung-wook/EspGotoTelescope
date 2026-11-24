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
#include "src/estimation/attitude_estimator.h"
#include "src/app/loop_logic.h"
#include "src/ui/simplegoto_ui.h"
#include "src/remote/remote_ui.h"
#include "src/config/settings_binding.h"
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
// Estimation
/************************************/
AttitudeEstimator att_est;
const float       ATT_EST_TAU_S = 1.0f;  // complementary filter crossover
astro::TargetTracker moon_tracker(astro::Target::Moon, 1000);     // Moon (lunar rate), once per second
astro::TargetTracker target_tracker(astro::Target::Mercury, 1000); // 1Target, once per second
loc::LastGpsRecorder last_gps;                                     // DES-LOC-02

/************************************/
// UI
/************************************/
SimpleGoToUi ui;
UiContext   ui_ctx;
Frame       frame;
int         rotary_count_prev = 0;

/************************************/
// Settings in flash (docs/REQUIREMENTS.md DES-CFG)
/************************************/
config::EepromSettingsStore settings_store;
config::Settings            settings;

static_assert(sizeof(frame.lines) == sizeof(monitor->msg), "Frame and OLED buffers must match");

/************************************/
// Performance and simple scheduler
/************************************/
Tictok_us perform_main;
Tictok_us perform_rest;
Tictok_us perform_imu;
Tictok_us perform_est;
Tictok_us perform_astro;
const float   hz             = MAIN_LOOP_HZ;
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
  bootLine(line, SIMPLEGOTO_NAME " v" SIMPLEGOTO_VERSION);
  char cpu_line[DISP_LINE_ROW];
  snprintf(cpu_line, sizeof(cpu_line), "1. Display ON %uMHz", ESP.getCpuFreqMHz());
  bootLine(line, cpu_line);   // 160 MHz expected (Tools > CPU Frequency)

  /************************************/
  // IMU - ICM20948
  /************************************/
  bootPending(line, "2. Intialize IMU");
  bool imu_on = imu.begin();
  bootLine(line, imu_on ? "2. IMU is ON" : ">> IMU failed");
  bootLine(line, imu.accelRangeText());
  bootLine(line, imu.gyroRangeText());
  bootLine(line, imu.magRateText());

  /************************************/
  // GPS
  /************************************/
  bootPending(line, "3. Intialize GPS");
  bool gps_on = gps->setup_gps();
  bootLine(line, gps_on ? "3. GPS is ON" : ">> GPS is timeout");

  /************************************/
  // Rotary encoder
  /************************************/
  bootPending(line, "4. Intialize Buttons");
  rotary.begin();
  rotary_count_prev = rotary.count();
  bootLine(line, "4. Buttons are ON");

  /************************************/
  // Motor
  /************************************/
  ra_motor.begin();
  bootLine(line, "5. GPIO/PWM is ON");

  /************************************/
  // Settings : calibration, RA duty, tracking offset (defaults if none or bad)
  /************************************/
  settings_store.begin();
  const config::LoadResult settings_result = config::load(settings_store, settings);
  config::applyToUi(settings, ui_ctx.state);
  ui_ctx.tel.settings_status = config::statusAfterLoad(settings_result);
  bootLine(line, config::loadText(settings_result));

#if REMOTE_UI_ENABLE
  /************************************/
  // Remote UI : Wi-Fi AP + web page + WebSocket
  /************************************/
  remote::begin();
  bootLine(line, "7. WiFi AP " REMOTE_UI_AP_SSID);
#endif

  att_est.begin(ATT_EST_TAU_S, 1.0f / hz);
  loop_timer.start(micros());
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
  t.altitude_m   = g->altitude;

  t.imu           = imu.data();
  t.loop_hz = hz;
  t.loop_us = perform_rest.getdt() + perform_main.getdt();
  t.main_us = perform_main.getdt();
  t.disp_us = monitor->perform_print.getdt();
  t.gps_us  = gps->perform_read.getdt();
  t.imu_us  = perform_imu.getdt();
  t.est_us  = perform_est.getdt();
  t.astro_us = perform_astro.getdt();
  t.remote   = remote::state();   // status bar Wi-Fi mark
#if VBAT_SENSE_ENABLE
  if (!battery.valid() || t.now_ms - battery_read_ms >= VBAT_PERIOD_MS) {
    battery.update(analogRead(A0));
    battery_read_ms = t.now_ms;
  }
#endif
  t.voltage_valid = battery.valid();
  t.voltage       = battery.volts();
}

void loop() {
  perform_main.tic();

  // Inputs
  int  count = rotary.count();
  int  delta = count - rotary_count_prev;
  rotary_count_prev = count;
  bool click = rotary.takeClick();
  // Phone (remote UI): dial steps add to the rotary, a click counts like the button
  int  remote_delta = 0;
  bool remote_click = false;
  remote::takeInput(remote_delta, remote_click);
  delta += remote_delta;
  click = click || remote_click;

  // Sensors
  gps->read_all();
  perform_imu.tic();
  imu.read();
  perform_imu.toc();
  updateTelemetry(ui_ctx.tel);
  app::correctImu(ui_ctx.tel, ui_ctx.state);

  // Estimation
  perform_est.tic();
  att_est.update(ui_ctx.tel.imu_corr, 1.0f / hz);
  perform_est.toc();
  ui_ctx.tel.roll_est_deg  = att_est.rollDeg();
  ui_ctx.tel.pitch_est_deg = att_est.pitchDeg();

  // Phone time from the remote UI page (DES-LOC-05), then position and time in use:
  // GPS, last GPS fix, manual, phone (DES-LOC)
  unsigned long phone_s = 0;
  int phone_ms = 0;
  if (remote::takePhoneTime(phone_s, phone_ms))
    app::setPhoneClock(ui_ctx.state, ui_ctx.tel.now_ms, phone_s, phone_ms);
  app::updateLocationTime(ui_ctx.tel, ui_ctx.state, last_gps);

  // Moon and target (each only on the frames where its 1 s period has passed),
  // then where the tube points (IMU on the tube, polar-aligned mount)
  perform_astro.tic();
  const bool astro_ran = app::updateAstro(moon_tracker, target_tracker, ui_ctx.tel, ui_ctx.state,
                                          IMU_TUBE_OPTICAL_AXIS, IMU_TUBE_DEC_AXIS);
  if (astro_ran) perform_astro.toc();

  // UI -> display
  frame.clear();
  ui.step(delta, click, ui_ctx, frame);
  monitor->setPower(!ui_ctx.state.display_off);   // Sleep: OLED off, phone still mirrors
  if (!ui_ctx.state.display_off) {
    memcpy(monitor->msg, frame.lines, sizeof(frame.lines));
    monitor->print();
  }
  remote::publish(frame);   // phone mirror (only when the lines changed)
  config::serviceRequests(settings_store, settings, ui_ctx);   // Save / Reset from the UI

  // Outputs (tracking overrides the manual RRcmd; calibration / Polar Align stop the motor)
  ra_motor.setSpeed(app::motorCommand(ui_ctx.tel, ui_ctx.state));

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
