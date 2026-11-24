#include "firmware_ui.h"

#include <cmath>
#include <cstdio>
#include <cstring>

// Firmware (read-only, include path Arduino/app/src)
#include "app/loop_logic.h"
#include "config/settings_binding.h"
#include "estimation/attitude_estimator.h"
#include "motor/ra_motor.h"
#include "sensors/imu_data.h"
#include "ui/telescope_ui.h"
#include "version.h"

namespace sim {

namespace {

// Same lines as setup() in Arduino/app/app.ino (ICM20948 set to 16G/250dps/10Hz,
// remote UI off).
void bootLines(const BootConfig& boot, const char* settings_text,
               const char* out[FirmwareUi::kBootLines]) {
  out[0] = SIMPLEGOTO_NAME " v" SIMPLEGOTO_VERSION;
  out[1] = "1. Display ON 160MHz";   // ESP.getCpuFreqMHz() at 160 MHz
  out[2] = boot.imu_ok ? "2. IMU is ON" : ">> IMU failed";
  out[3] = "Acc +-16G";
  out[4] = "Gyro 250 dps";
  out[5] = "Mag 10 Hz";
  out[6] = boot.gps_ok ? "3. GPS is ON" : ">> GPS is timeout";
  out[7] = "4. Buttons are ON";
  out[8] = "5. GPIO/PWM is ON";
  out[9] = settings_text;
}

// updateTelemetry() of app.ino, with the fake sensors as the source.
void fillTelemetry(unsigned long now_ms, const SensorState& s, const UiState& st, Telemetry& t) {
  const GpsState& g = s.gps;
  t.now_ms = now_ms;
  t.year = g.year;
  t.month = g.month;
  t.day = g.day;
  t.hour = g.hour;
  t.minute = g.minute;
  t.seconds = g.seconds;
  t.milliseconds = g.milliseconds;
  t.fixquality = g.fixquality;
  t.satellites = g.satellites;
  // Adafruit_GPS: lat/lon hemisphere letters; latitude_fixed*1e-7 is the
  // signed angle (negative for S/W).
  t.lat_hemi = g.latitude >= 0 ? 'N' : 'S';
  t.lon_hemi = g.longitude >= 0 ? 'E' : 'W';
  t.latitude = g.latitude;
  t.longitude = g.longitude;

  const ImuState& m = s.imu;
  t.imu.ax = (float)m.ax;
  t.imu.ay = (float)m.ay;
  t.imu.az = (float)m.az;
  t.imu.wx = (float)m.wx;
  t.imu.wy = (float)m.wy;
  t.imu.wz = (float)m.wz;
  t.imu.mx = (float)m.mx;
  t.imu.my = (float)m.my;
  t.imu.mz = (float)m.mz;
  t.imu.temperature = (float)m.temperature;

  t.voltage_valid = true;        // the simulator always "measures" the supply
  t.voltage = (float)s.voltage;
  t.remote = s.remote;

  // Health: typical numbers from the board (the simulator does not time it)
  t.loop_hz = 10.0f;
  t.loop_us = 100000;
  t.main_us = 24300;
  t.disp_us = 21800;
  t.gps_us = 650;
  t.imu_us = 1250;
  t.est_us = 180;
  t.astro_us = 4000;   // MoonState, about 4 ms at 160 MHz (tools/astro_bench measures it)
}

}  // namespace

FirmwareUi::FirmwareUi()
    : store_(new config::RamSettingsStore()), settings_(new config::Settings()) {}
FirmwareUi::~FirmwareUi() {}

void FirmwareUi::begin(const BootConfig& boot, const SensorState&) {
  boot_ = boot;
  boot_frame_ = 0;
  last_count_ = 0;
  // Fresh UI and context, like a power cycle (TelescopeUi has no reset()).
  ui_.reset(new TelescopeUi());
  ctx_.reset(new UiContext());
  att_est_.reset(new AttitudeEstimator());
  moon_tracker_.reset(new astro::TargetTracker(astro::Target::Moon, 1000));
  target_tracker_.reset(new astro::TargetTracker(astro::Target::Mercury, 1000));
  last_gps_.reset(new loc::LastGpsRecorder());
  // setup(): settings after the motor, before the main loop
  const config::LoadResult r = config::load(*store_, *settings_);
  config::applyToUi(*settings_, ctx_->state);
  ctx_->tel.settings_status = config::statusAfterLoad(r);
  settings_load_ = (int)r;
  att_est_->begin(kAttEstTauS, kFrameMs / 1000.0f);   // end of setup()
}

void FirmwareUi::renderBoot(Frame& out) {
  const char* lines[kBootLines];
  bootLines(boot_, config::loadText((config::LoadResult)settings_load_), lines);
  int shown = boot_frame_ / kBootLineFrames + 1;
  if (shown > kBootLines) shown = kBootLines;
  // bootLine() of app.ino: line 0 stays, the rest scrolls up when the screen is full
  const int skip = shown > kLines ? shown - kLines : 0;
  for (int row = 0; row < kLines && row < shown; row++) {
    const int i = row == 0 ? 0 : row + skip;
    std::snprintf(out.lines[row], kCols + 1, "%s", lines[i]);
  }
}

void FirmwareUi::frame(unsigned long now_ms, const InputSnapshot& input,
                       const SensorState& sensors, Frame& out, MotorOutput& motor) {
  // rotary_count_prev logic of loop(); setup() takes the count at boot end,
  // so turns and clicks during boot are dropped.
  int delta = input.count - last_count_;
  last_count_ = input.count;

  if (booting()) {
    renderBoot(out);
    boot_frame_++;
    motor = MotorOutput();   // ra_motor.begin(): stopped
    return;
  }

  fillTelemetry(now_ms, sensors, ctx_->state, ctx_->tel);
  app::correctImu(ctx_->tel, ctx_->state);
  // Estimation, as in loop()
  att_est_->update(ctx_->tel.imu_corr, kFrameMs / 1000.0f);
  ctx_->tel.roll_est_deg = att_est_->rollDeg();
  ctx_->tel.pitch_est_deg = att_est_->pitchDeg();
  if (input.phone_time)
    app::setPhoneClock(ctx_->state, now_ms, input.phone_unix_s, input.phone_ms);
  app::updateLocationTime(ctx_->tel, ctx_->state, *last_gps_);
  app::updateAstro(*moon_tracker_, *target_tracker_, ctx_->tel, ctx_->state,
                   IMU_TUBE_OPTICAL_AXIS, IMU_TUBE_DEC_AXIS);

  ::Frame f;   // the firmware's frame type
  ui_->step(delta, input.clicked(), *ctx_, f);
  static_assert(sizeof(f.lines[0]) == kCols + 1, "line width must match");
  for (int i = 0; i < kLines; i++) {
    std::memcpy(out.lines[i], f.lines[i], kCols + 1);
    out.lines[i][kCols] = '\0';
  }

  config::serviceRequests(*store_, *settings_, *ctx_);   // Save / Reset from the UI
  ::MotorOutput m = RaMotor::toOutput(app::motorCommand(ctx_->tel, ctx_->state));
  motor.in1 = m.in1;
  motor.in2 = m.in2;
  motor.pwm = m.duty;
}

}  // namespace sim
