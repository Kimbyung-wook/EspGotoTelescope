/************************************/
// Automatic Telescope - NodeMCU v3 (ESP8266)
//   OLED SSD1306, rotary encoder, ICM20948 IMU, Adafruit Ultimate GPS,
//   RA axis DC motor on an H-bridge
/************************************/
#include "src/board_config.h"

#if defined(F_CPU) && F_CPU != 160000000L
#warning "Build at 160 MHz (Arduino IDE: Tools > CPU Frequency > 160 MHz, or scripts/build.sh)"
#endif
#include "src/display/oled.h"
#include "src/gps/gps.h"
#include "src/util/tictok_us.h"
#include "src/util/loop_timer.h"
#include "src/input/rotary.h"
#include "src/sensors/imu.h"
#include "src/motor/ra_motor.h"
#include "src/estimation/attitude_estimator.h"
#include "src/astro/target_telemetry.h"
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
// Estimation
/************************************/
AttitudeEstimator att_est;
const float       ATT_EST_TAU_S = 1.0f;  // complementary filter crossover
astro::TargetTracker moon_tracker(astro::Target::Moon, 1000);     // 9Axis for Moon, once per second
astro::TargetTracker target_tracker(astro::Target::Mercury, 1000); // 10Planet Tracking, once per second

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
Tictok_us perform_est;
Tictok_us perform_astro;
const float   hz             = MAIN_LOOP_HZ;
LoopTimer     loop_timer((unsigned long)(1000000.0f / hz));

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
  char cpu_line[DISP_LINE_ROW];
  snprintf(cpu_line, sizeof(cpu_line), "1. Display ON %uMHz", ESP.getCpuFreqMHz());
  bootLine(line, cpu_line);   // 160 MHz expected (Tools > CPU Frequency)

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
  t.imu_corr      = withoutGyroBias(t.imu, ui_ctx.state.gyro_bias);
  t.roll_acc_deg  = rollFromAccelDeg(t.imu);
  t.pitch_acc_deg = pitchFromAccelDeg(t.imu);
  t.loop_hz = hz;
  t.loop_us = perform_rest.getdt() + perform_main.getdt();
  t.main_us = perform_main.getdt();
  t.disp_us = monitor->perform_print.getdt();
  t.gps_us  = gps->perform_read.getdt();
  t.imu_us  = perform_imu.getdt();
  t.est_us  = perform_est.getdt();
  t.astro_us = perform_astro.getdt();
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

  // Estimation
  perform_est.tic();
  att_est.update(ui_ctx.tel.imu_corr, 1.0f / hz);
  perform_est.toc();
  ui_ctx.tel.roll_est_deg  = att_est.rollDeg();
  ui_ctx.tel.pitch_est_deg = att_est.pitchDeg();

  // Moon and tracking target (each only on the frames where its 1 s period has passed)
  perform_astro.tic();
  bool astro_ran = astro::updateMoonTelemetry(moon_tracker, ui_ctx.tel);
  astro_ran |= astro::updateTargetTelemetry(target_tracker, ui_ctx.state.track_target, ui_ctx.tel);
  if (astro_ran) perform_astro.toc();
  ui_ctx.tel.ra_calibrated = RA_DUTY_AT_SIDEREAL > 0.0f;

  // UI -> display
  frame.clear();
  ui.step(delta, click, ui_ctx, frame);
  memcpy(monitor->msg, frame.lines, sizeof(frame.lines));
  monitor->print();

  // Outputs (tracking overrides the manual RRcmd while it is on)
  astro::applyTracking(ui_ctx.tel, ui_ctx.state, RA_DUTY_AT_SIDEREAL);
  ra_motor.setSpeed(ui_ctx.state.ra_speed_cmd);

  perform_main.toc();

  /************************************/
  // Simple scheduler : wait for the next period
  /************************************/
  perform_rest.tic();
  while (!loop_timer.due(micros())) {
    yield();
  }
  perform_rest.toc();
}
