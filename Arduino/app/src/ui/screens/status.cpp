#include "status.h"
#include "../../gps/gps_link.h"
#include "../../location/location.h"

void GpsStatusScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  char fix[6];
  gpsFixText(t.gps.fixquality, fix, sizeof(fix));
  renderTitle(f);
  f.set(2, "Date: 20%02d/%02d/%02d", t.gps.year % 100, t.gps.month % 100, t.gps.day % 100);
  f.set(3, "Time:   %02d:%02d:%02d.%03d", t.gps.hour % 100, t.gps.minute % 100, t.gps.seconds % 100, t.gps.milliseconds % 1000);
  f.set(4, "%5s Sat %2d", fix, t.gps.satellites);
  f.set(5, "Pos %-4s Time %s", loc::sourceText(t.obs.loc_source), loc::timeSourceText(t.obs.time_source));
  f.set(6, " %c %12.8f DEG", t.gps.lat_hemi, t.gps.latitude);
  f.set(7, " %c %12.8f DEG", t.gps.lon_hemi, t.gps.longitude);
}

void AccGyroScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const ImuData& d = ctx.tel.imu.raw;
  renderTitle(f);
  f.set(2, "Ax    %10.5f mpss", d.ax);
  f.set(3, "Ay    %10.5f mpss", d.ay);
  f.set(4, "Az    %10.5f mpss", d.az);
  f.set(5, "Wx    %10.5f dps",  d.wx);
  f.set(6, "Wy    %10.5f dps",  d.wy);
  f.set(7, "Wz    %10.5f dps",  d.wz);
}

void MagTempScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const ImuData& d = ctx.tel.imu.raw;
  renderTitle(f);
  f.set(2, "Mx   %10.5f uT", d.mx);
  f.set(3, "My   %10.5f uT", d.my);
  f.set(4, "Mz   %10.5f uT", d.mz);
  f.set(5, "Temp %10.5f 'c ", d.temperature);
  f.blank(6);
}

void ImuCalibStatusScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const UiState& s = ctx.state;
  renderTitle(f);
  f.set(2, "Ax b%+8.4f s%6.4f", s.cfg.accel_bias[0], s.cfg.accel_scale[0]);
  f.set(3, "Ay b%+8.4f s%6.4f", s.cfg.accel_bias[1], s.cfg.accel_scale[1]);
  f.set(4, "Az b%+8.4f s%6.4f", s.cfg.accel_bias[2], s.cfg.accel_scale[2]);
  f.set(5, "bWx  %10.5f dps",  s.cfg.gyro_bias[0]);
  f.set(6, "bWy  %10.5f dps",  s.cfg.gyro_bias[1]);
  f.set(7, "bWz  %10.5f dps",  s.cfg.gyro_bias[2]);
}

void AttEstScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  renderTitle(f);
  f.set(2, "ROLL   %10.5f DEG", t.imu.roll_est_deg);
  f.set(3, "PITCH  %10.5f DEG", t.imu.pitch_est_deg);
  f.set(4, "aR %7.2f aP %7.2f", t.imu.roll_acc_deg, t.imu.pitch_acc_deg);
  f.set(5, "Wx     %10.5f DPS", t.imu.corr.wx);
  f.set(6, "Wy     %10.5f DPS", t.imu.corr.wy);
  f.set(7, "Wz     %10.5f DPS", t.imu.corr.wz);
}

void HealthScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  renderTitle(f);
  f.set(2, "Loop %6lu/%5.1fHz", t.health.loop_us, t.health.loop_hz);
  f.set(3, "MAIN %6lu", t.health.main_us);
  f.set(4, "DISP %6lu", t.health.disp_us);
  f.set(5, "GPS  %6lu", t.health.gps_us);
  f.set(6, "IMU  %6lu", t.health.imu_us);
  f.set(7, "EST  %6lu AST %5lu", t.health.est_us, t.health.astro_us);
}

void SleepScreen::render(Frame& f, const UiContext& /*ctx*/, char /*blink*/) {
  f.blank(1);
}
