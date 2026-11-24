#include "firmware_ui.h"

#include <cmath>
#include <cstdio>
#include <cstring>

// Firmware (read-only, include path Arduino/app/src)
#include "app/boot_text.h"
#include "app/core.h"
#include "motor/ra_motor.h"
#include "sensors/imu_ranges.h"

namespace sim {

namespace {

// Same lines as setup() in Arduino/app/app.ino (remote UI off)
void bootLines(const BootConfig& boot, const char* settings_text, char display_line[kCols + 1],
               const char* out[FirmwareUi::kBootLines]) {
  boot::displayLine(display_line, kCols + 1, 160);   // ESP.getCpuFreqMHz() at 160 MHz
  out[0] = boot::TITLE;
  out[1] = display_line;
  out[2] = boot.imu_ok ? boot::IMU_ON : boot::IMU_FAILED;
  out[3] = imuAccelRangeText(IMU_ACCEL_RANGE_G);
  out[4] = imuGyroRangeText(IMU_GYRO_RANGE_DPS);
  out[5] = imuMagRateText(IMU_MAG_RATE_HZ);
  out[6] = boot.gps_ok ? boot::GPS_ON : boot::GPS_TIMEOUT;
  out[7] = boot::BUTTONS_ON;
  out[8] = boot::MOTOR_ON;
  out[9] = settings_text;
}

// readInputs() of app.ino, with the fake sensors as the source.
app::FrameInput toInput(unsigned long now_ms, int delta, const InputSnapshot& input,
                        const SensorState& s) {
  app::FrameInput in;
  in.now_ms = now_ms;
  in.delta = delta;
  in.click = input.clicked();
  const GpsState& g = s.gps;
  in.gps.year = g.year;
  in.gps.month = g.month;
  in.gps.day = g.day;
  in.gps.hour = g.hour;
  in.gps.minute = g.minute;
  in.gps.seconds = g.seconds;
  in.gps.milliseconds = g.milliseconds;
  in.gps.fixquality = g.fixquality;
  in.gps.satellites = g.satellites;
  in.gps.hdop = (float)g.hdop;
  in.gps.link = g.link;
  // Adafruit_GPS: lat/lon hemisphere letters; latitude_fixed*1e-7 is the
  // signed angle (negative for S/W).
  in.gps.lat_hemi = g.latitude >= 0 ? 'N' : 'S';
  in.gps.lon_hemi = g.longitude >= 0 ? 'E' : 'W';
  in.gps.latitude = g.latitude;
  in.gps.longitude = g.longitude;

  const ImuState& m = s.imu;
  in.imu.ax = (float)m.ax;
  in.imu.ay = (float)m.ay;
  in.imu.az = (float)m.az;
  in.imu.wx = (float)m.wx;
  in.imu.wy = (float)m.wy;
  in.imu.wz = (float)m.wz;
  in.imu.mx = (float)m.mx;
  in.imu.my = (float)m.my;
  in.imu.mz = (float)m.mz;
  in.imu.temperature = (float)m.temperature;

  in.power.valid = true;        // the simulator always "measures" the supply
  in.power.volts = (float)s.voltage;
  in.remote = s.remote;

  // Health: typical numbers from the board (the simulator does not time it)
  in.health.loop_hz = 10.0f;
  in.health.loop_us = 100000;
  in.health.main_us = 24300;
  in.health.disp_us = 21800;
  in.health.gps_us = 650;
  in.health.imu_us = 1250;
  in.health.est_us = 180;
  in.health.astro_us = 4000;   // targetState(Moon), about 4 ms at 160 MHz (tools/astro_bench measures it)

  in.phone_time = input.phone_time;
  in.phone_unix_s = input.phone_unix_s;
  in.phone_ms = input.phone_ms;
  return in;
}

}  // namespace

FirmwareUi::FirmwareUi()
    : store_(new config::RamSettingsStore()) {}
FirmwareUi::~FirmwareUi() {}

void FirmwareUi::begin(const BootConfig& boot, const SensorState&) {
  boot_ = boot;
  boot_frame_ = 0;
  last_count_ = 0;
  // Fresh core, like a power cycle (no clock: Health shows the typical numbers)
  core_.reset(new app::Core(1000.0f / kFrameMs));
  settings_load_ = (int)core_->begin(*store_);   // setup(): settings, estimator start
}

SimpleGoToUi& FirmwareUi::ui() { return core_->ui(); }
UiContext& FirmwareUi::context() { return core_->context(); }

void FirmwareUi::renderBoot(Frame& out) {
  const char* lines[kBootLines];
  char display_line[kCols + 1];
  bootLines(boot_, config::loadText((config::LoadResult)settings_load_), display_line, lines);
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

  ::Frame f;   // the firmware's frame type
  core_->step(toInput(now_ms, delta, input, sensors), f);
  static_assert(sizeof(f.lines[0]) == kCols + 1, "line width must match");
  for (int i = 0; i < kLines; i++) {
    std::memcpy(out.lines[i], f.lines[i], kCols + 1);
    out.lines[i][kCols] = '\0';
  }

  ::MotorOutput m = RaMotor::toOutput(core_->finish(*store_));
  motor.in1 = m.in1;
  motor.in2 = m.in2;
  motor.pwm = m.duty;
}

}  // namespace sim
