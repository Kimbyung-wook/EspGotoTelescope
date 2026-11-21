#include "screens.h"

/************************************/
// Main menu
/************************************/
MainMenuScreen::MainMenuScreen(const char* const labels[ITEMS], Screen* const targets[ITEMS])
  : labels_(labels), targets_(targets), cursor_(ITEMS) {}

void MainMenuScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  Screen* target = targets_[cursor_.selected()];
  if (target) mgr.push(target, ctx);
}

void MainMenuScreen::render(Frame& f, const UiContext& /*ctx*/, char blink) {
  f.set(1, "Main Menu");
  int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    int i = first + row;
    f.set(Frame::CONTENT_FIRST + row, "%c%s", cursor_.mark(i, blink), labels_[i]);
  }
}

/************************************/
// RA speed
/************************************/
void RaSpeedScreen::onRotate(int delta, UiContext& ctx) {
  if (editing_) ctx.state.ra_speed_cmd += 1.0f * delta;
  else          cursor_.move(delta);
}

void RaSpeedScreen::onClick(ScreenManager& mgr, UiContext& /*ctx*/) {
  if (editing_) {                          // finish editing, cursor back on RRcmd
    editing_ = false;
    cursor_.reset();
    return;
  }
  switch (cursor_.selected()) {
    case 0: editing_ = true; break;        // RRcmd
    case 2: mgr.pop();       break;        // Quit
    default: break;                        // RRate : read only
  }
}

void RaSpeedScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const Telemetry& t = ctx.tel;
  if (editing_) {
    f.set(1, "*Set RA Speed");
    f.set(2, "%cRRcmd %10.5fDEG", blink, ctx.state.ra_speed_cmd);
    f.set(3, " RRate %10.5fDPS", t.imu.wx);
    f.set(4, " Exit");
  } else {
    f.set(1, "*%s", title_);
    f.set(2, "%cRRcmd %10.5fDEG", cursor_.mark(0, blink), ctx.state.ra_speed_cmd);
    f.set(3, "%cRRate %10.5fDPS", cursor_.mark(1, blink), t.imu.wx);
    f.set(4, "%cQuit",            cursor_.mark(2, blink));
  }
  f.set(5, " ROL %10.5f  DEG", t.roll_acc_deg);
  f.set(6, " LAT %c%10.6fDEG", t.lat_hemi, t.latitude);
  f.set(7, " PIT %10.5f  DEG", t.pitch_acc_deg);
}

/************************************/
// Info screens
/************************************/
static void fixText(int q, char* out, int size) {
  switch (q) {
    case 0:  snprintf(out, size, "NoFix"); break;
    case 1:  snprintf(out, size, "GPS");   break;
    case 2:  snprintf(out, size, "DGPS");  break;
    default: snprintf(out, size, "Q%d", q); break;
  }
}

void GpsStatusScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  char fix[6];
  fixText(t.fixquality, fix, sizeof(fix));
  renderTitle(f);
  f.set(2, "Date: 20%02d/%02d/%02d", t.year % 100, t.month % 100, t.day % 100);
  f.set(3, "Time:   %02d:%02d:%02d.%03d", t.hour % 100, t.minute % 100, t.seconds % 100, t.milliseconds % 1000);
  f.set(4, "%5s Sat %2d", fix, t.satellites);
  f.set(5, " ");
  f.set(6, " %c %12.8f DEG", t.lat_hemi, t.latitude);
  f.set(7, " %c %12.8f DEG", t.lon_hemi, t.longitude);
}

void AccGyroScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const ImuData& d = ctx.tel.imu;
  renderTitle(f);
  f.set(2, "Ax    %10.5f mpss", d.ax);
  f.set(3, "Ay    %10.5f mpss", d.ay);
  f.set(4, "Az    %10.5f mpss", d.az);
  f.set(5, "Wx    %10.5f dps",  d.wx);
  f.set(6, "Wy    %10.5f dps",  d.wy);
  f.set(7, "Wz    %10.5f dps",  d.wz);
}

void MagTempScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const ImuData& d = ctx.tel.imu;
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
  f.set(2, "bAx  %10.5f mpss", s.accel_bias[0]);
  f.set(3, "bAy  %10.5f mpss", s.accel_bias[1]);
  f.set(4, "bAz  %10.5f mpss", s.accel_bias[2]);
  f.set(5, "bWx  %10.5f dps",  s.gyro_bias[0]);
  f.set(6, "bWy  %10.5f dps",  s.gyro_bias[1]);
  f.set(7, "bWz  %10.5f dps",  s.gyro_bias[2]);
}

void AttEstScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  renderTitle(f);
  f.set(2, "ROLL   %10.5f DEG", t.roll_est_deg);
  f.set(3, "PITCH  %10.5f DEG", t.pitch_est_deg);
  f.set(4, "aR %7.2f aP %7.2f", t.roll_acc_deg, t.pitch_acc_deg);
  f.set(5, "Wx     %10.5f DPS", t.imu_corr.wx);
  f.set(6, "Wy     %10.5f DPS", t.imu_corr.wy);
  f.set(7, "Wz     %10.5f DPS", t.imu_corr.wz);
}

void HealthScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  renderTitle(f);
  f.set(2, "Loop %6lu/%5.1fHz", t.loop_us, t.loop_hz);
  f.set(3, "MAIN %6lu", t.main_us);
  f.set(4, "DISP %6lu", t.disp_us);
  f.set(5, "GPS  %6lu", t.gps_us);
  f.set(6, "IMU  %6lu", t.imu_us);
  f.set(7, "EST  %6lu", t.est_us);
}

void MoonAxisScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  renderTitle(f);
  f.set(2, "ROLL  %10.5f DEG", t.roll_acc_deg);
  f.set(3, "PITCH %10.5f DEG", t.pitch_acc_deg);
  f.set(4, "YAW   %10.5f DEG", 120.0012f);   // placeholder
  f.set(5, "-- Moon --------");
  f.set(6, "R.A.  %10.5f DEG",  37.0013f);   // placeholder
  f.set(7, "DEC.  %10.5f DEG",-120.0014f);   // placeholder
}

void SleepScreen::render(Frame& f, const UiContext& /*ctx*/, char /*blink*/) {
  f.blank(1);
}

/************************************/
// IMU calibration
/************************************/
void ImuCalibMenuScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  switch (cursor_.selected()) {
    case 2: mgr.push(&gyro_bias_, ctx); break;   // Gyro Bias Calib
    case 5: mgr.pop();                  break;   // Quit
    default: break;                              // not implemented yet
  }
}

void ImuCalibMenuScreen::render(Frame& f, const UiContext& /*ctx*/, char blink) {
  f.set(1, "*%s", title_);
  f.set(2, "%cAcc. Bias Calib",  cursor_.mark(0, blink));
  f.set(3, "%cAcc. Scale Calib", cursor_.mark(1, blink));
  f.set(4, "%cGyro Bias Calib",  cursor_.mark(2, blink));
  f.set(5, "%cGyro Scale Calib", cursor_.mark(3, blink));
  f.set(6, "%c",                 cursor_.mark(4, blink));
  f.set(7, "%cQuit",             cursor_.mark(5, blink));
}

// 0-1 s : take the current rate, 1-6 s : fast average (a=0.3), 6-16 s : slow average (a=0.01)
void GyroBiasCalibScreen::tick(UiContext& ctx) {
  const float w[3] = {ctx.tel.imu.wx, ctx.tel.imu.wy, ctx.tel.imu.wz};
  calib_.update(ctx.tel.now_ms, w);
  for (int i = 0; i < 3; i++) ctx.state.gyro_bias[i] = calib_.bias()[i];
}

void GyroBiasCalibScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const ImuData& d = ctx.tel.imu;
  const float* b = ctx.state.gyro_bias;
  f.set(1, "%s", GyroBiasCalibrator::phaseText(calib_.phase()));
  f.set(2, "Wx %8.5f->%8.5f", d.wx, d.wx - b[0]);
  f.set(3, "Wy %8.5f->%8.5f", d.wy, d.wy - b[1]);
  f.set(4, "Wz %8.5f->%8.5f", d.wz, d.wz - b[2]);
  f.set(5, "bWx   %10.5f dps", b[0]);
  f.set(6, "bWy   %10.5f dps", b[1]);
  f.set(7, "bWz   %10.5f dps", b[2]);
}
