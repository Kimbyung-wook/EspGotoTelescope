#include "screens.h"
#include <math.h>
#include "../app/tracking.h"
#include "../location/location.h"
#include "../gps/gps_link.h"
#include "../board_config.h"

/************************************/
// Menus
/************************************/
void MenuScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  Screen* target = targets_[cursor_.selected()];
  if (target) mgr.push(target, ctx);
  else        mgr.pop();                     // Back
}

void MenuScreen::render(Frame& f, const UiContext& /*ctx*/, char blink) {
  if (root_) f.set(1, "%s", title_);
  else       f.set(1, "*%s", title_);
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int i = first + row;
    if (i >= cursor_.count()) { f.blank(Frame::CONTENT_FIRST + row, Frame::CONTENT_FIRST + row); continue; }
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

void RaSpeedScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  if (editing_) {                          // finish editing, cursor back on RRcmd
    editing_ = false;
    cursor_.reset();
    return;
  }
  switch (cursor_.selected()) {
    case 0: editing_ = true; ctx.state.tracking = false; break;   // RRcmd : manual speed
    case 2: mgr.pop();       break;        // Quit
    default: break;                        // RRate : read only
  }
}

void RaSpeedScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const Telemetry& t = ctx.tel;
  if (editing_) {
    f.set(1, "*Set RA Speed");
    f.set(2, "%cRRcmd %10.5fDEG", blink, ctx.state.ra_speed_cmd);
    f.set(3, " RRate %10.5fDPS", t.imu_corr.wx);
    f.set(4, " Exit");
  } else {
    f.set(1, "*%s", title_);
    f.set(2, "%cRRcmd %10.5fDEG", cursor_.mark(0, blink), ctx.state.ra_speed_cmd);
    f.set(3, "%cRRate %10.5fDPS", cursor_.mark(1, blink), t.imu_corr.wx);
    f.set(4, "%cQuit",            cursor_.mark(2, blink));
  }
  f.set(5, " ROL %10.5f  DEG", t.roll_acc_deg);
  f.set(6, " LAT %c%10.6fDEG", t.obs_lat_deg >= 0.0 ? 'N' : 'S', fabs(t.obs_lat_deg));
  f.set(7, " PIT %10.5f  DEG", t.pitch_acc_deg);
}

/************************************/
// Info screens
/************************************/
void GpsStatusScreen::render(Frame& f, const UiContext& ctx, char /*blink*/) {
  const Telemetry& t = ctx.tel;
  char fix[6];
  gpsFixText(t.fixquality, fix, sizeof(fix));
  renderTitle(f);
  f.set(2, "Date: 20%02d/%02d/%02d", t.year % 100, t.month % 100, t.day % 100);
  f.set(3, "Time:   %02d:%02d:%02d.%03d", t.hour % 100, t.minute % 100, t.seconds % 100, t.milliseconds % 1000);
  f.set(4, "%5s Sat %2d", fix, t.satellites);
  f.set(5, "Pos %-4s Time %s", loc::sourceText(t.loc_source), loc::timeSourceText(t.time_source));
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
  f.set(2, "Ax b%+8.4f s%6.4f", s.accel_bias[0], s.accel_scale[0]);
  f.set(3, "Ay b%+8.4f s%6.4f", s.accel_bias[1], s.accel_scale[1]);
  f.set(4, "Az b%+8.4f s%6.4f", s.accel_bias[2], s.accel_scale[2]);
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
  f.set(7, "EST  %6lu AST %5lu", t.est_us, t.astro_us);
}

void SleepScreen::render(Frame& f, const UiContext& /*ctx*/, char /*blink*/) {
  f.blank(1);
}

/************************************/
// Gyro calibration (DES-CAL-03)
/************************************/
void GyroCalibScreen::tick(UiContext& ctx) {
  ctx.state.motor_inhibit = true;
  calib_.update(ctx.tel.now_ms, ctx.tel.imu);
}

void GyroCalibScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  switch (calib_.state()) {
    case cal::GyroCalibration::RESULT:
      if (choice_.first()) {                  // Apply : used from now on and saved
        for (int i = 0; i < 3; i++) ctx.state.gyro_bias[i] = calib_.bias(i);
        ctx.state.settings_save_request = true;
      }
      mgr.pop();
      break;
    case cal::GyroCalibration::FAILED:
      if (choice_.first()) { calib_.start(ctx.tel.now_ms); choice_.reset(); }   // Retry
      else mgr.pop();
      break;
    default:
      mgr.pop();                              // Cancel : old bias stays
      break;
  }
}

void GyroCalibScreen::render(Frame& f, const UiContext& ctx, char blink) {
  using G = cal::GyroCalibration;
  const ImuData& d = ctx.tel.imu;
  f.set(1, "*Gyro Calib");
  switch (calib_.state()) {
    case G::FAILED:
      f.set(2, "Shock detected.");
      f.set(3, "Keep the mount still");
      f.set(4, "and retry.");
      f.blank(5, 6);
      choice_.render(f, 7, "Retry", "Quit", blink);
      return;
    case G::RESULT:
      f.set(2, "Done. Apply?");
      break;
    case G::MEASURE:
      f.set(2, "Measuring %2d/%2d s", calib_.elapsedS(), calib_.totalS());
      break;
    default:
      f.set(2, "Keep still  0/%2d s", calib_.totalS());
      f.set(3, "Wx  %+10.5f dps", d.wx);
      f.set(4, "Wy  %+10.5f dps", d.wy);
      f.set(5, "Wz  %+10.5f dps", d.wz);
      f.blank(6);
      f.set(7, "%cCancel", blink);
      return;
  }
  f.set(3, "bWx %+10.5f dps", calib_.bias(0));
  f.set(4, "bWy %+10.5f dps", calib_.bias(1));
  f.set(5, "bWz %+10.5f dps", calib_.bias(2));
  f.set(6, "sd %5.3f %5.3f %5.3f", calib_.stddev(0), calib_.stddev(1), calib_.stddev(2));
  if (calib_.state() == G::RESULT) choice_.render(f, 7, "Apply", "Discard", blink);
  else                             f.set(7, "%cCancel", blink);
}

/************************************/
// Accelerometer six-position calibration (DES-CAL-04..06)
/************************************/
void AccelCalibScreen::tick(UiContext& ctx) {
  ctx.state.motor_inhibit = true;
  calib_.update(ctx.tel.now_ms, ctx.tel.imu);
}

void AccelCalibScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  switch (calib_.state()) {
    case cal::AccelCalibration::RESULT:
      if (choice_.first()) {                  // Apply
        for (int i = 0; i < 3; i++) {
          ctx.state.accel_bias[i] = calib_.bias(i);
          ctx.state.accel_scale[i] = calib_.scale(i);
        }
        ctx.state.settings_save_request = true;
      }
      mgr.pop();
      break;
    case cal::AccelCalibration::FAILED:
      if (choice_.first()) { calib_.start(ctx.tel.now_ms); choice_.reset(); }   // Retry
      else mgr.pop();
      break;
    default:
      mgr.pop();                              // Cancel
      break;
  }
}

void AccelCalibScreen::render(Frame& f, const UiContext& ctx, char blink) {
  using A = cal::AccelCalibration;
  const ImuData& d = ctx.tel.imu;
  f.set(1, "*Accel Calib");
  if (calib_.state() == A::RESULT || calib_.state() == A::FAILED) {
    const bool ok = calib_.state() == A::RESULT;
    f.set(2, ok ? "Done. Apply?" : "Bad result. Retry");
    f.set(3, "X b%+8.4f s%6.4f", calib_.bias(0), calib_.scale(0));
    f.set(4, "Y b%+8.4f s%6.4f", calib_.bias(1), calib_.scale(1));
    f.set(5, "Z b%+8.4f s%6.4f", calib_.bias(2), calib_.scale(2));
    f.blank(6);
    if (ok) choice_.render(f, 7, "Apply", "Discard", blink);
    else    choice_.render(f, 7, "Retry", "Quit", blink);
    return;
  }
  const char* axis = A::stepName(calib_.step());
  f.set(2, "Step %d/%d %s up", calib_.step() + 1, A::STEPS, axis);
  switch (calib_.state()) {
    case A::SETTLE:  f.set(3, "Hold still..."); break;
    case A::MEASURE: f.set(3, "Measuring %d/%d s", calib_.measuredS(), (int)(A::MEASURE_MS / 1000)); break;
    default:
      if (calib_.shockInStep()) f.set(3, "Moved! Redo %s up", axis);
      else                      f.set(3, "Turn %s up, hold", axis);
      break;
  }
  f.set(4, "a %+5.1f %+5.1f %+5.1f", d.ax, d.ay, d.az);
  f.set(5, "Detach IMU, lay it");
  f.set(6, "flat, same axes back");
  f.set(7, "%cCancel", blink);
}

/************************************/
// Polar alignment
/************************************/
void PolarAlignScreen::tick(UiContext& ctx) {
  ctx.state.motor_inhibit = true;            // DES-PA-01
  const Telemetry& t = ctx.tel;
  // DES-PA-04 : altitude only for now (azimuth needs the magnetometer, DES-MAG)
  const bool inside = t.loc_source != loc::SRC_NONE && fabs(t.tube_alt_deg - t.obs_lat_deg) <= PA_ALT_TOL_DEG;
  if (!inside) { holding_ = false; ok_ = false; return; }
  if (!holding_) { holding_ = true; since_ms_ = t.now_ms; }
  ok_ = (unsigned long)(t.now_ms - since_ms_) >= (unsigned long)PA_OK_HOLD_MS;
}

void PolarAlignScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  if (cursor_.viewingInfo()) return;
  if (cursor_.selected() == BACK) { mgr.pop(); return; }
  if (ctx.state.polar_aligned) ctx.state.polar_aligned = false;   // Clear
  else if (ok_)                ctx.state.polar_aligned = true;    // Confirm
}

void PolarAlignScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const Telemetry& t = ctx.tel;
  const bool have_lat = t.loc_source != loc::SRC_NONE;
  const double dalt = t.tube_alt_deg - t.obs_lat_deg;
  f.set(1, "*%s", title_);
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int line = Frame::CONTENT_FIRST + row;
    switch (first + row) {
      case 0: f.set(line, "Dec 90: tube // axis"); break;
      case 1:
        if (have_lat) f.set(line, "ALT%+7.2f LAT%+7.2f", t.tube_alt_deg, t.obs_lat_deg);
        else          f.set(line, "ALT%+7.2f LAT    ---", t.tube_alt_deg);
        break;
      case 2:
        if (!have_lat) f.set(line, "dALT   --- no loc");
        else f.set(line, "dALT %+6.2f %s", dalt,
                   fabs(dalt) <= PA_ALT_TOL_DEG ? "OK" : (dalt < 0.0 ? "UP" : "DOWN"));
        break;
      case 3: f.set(line, "dAZ    --- no mag"); break;
      case 4:
        if (t.polaris_valid) {
          const double h = astro::polarisClockHours(t.polaris_ha_deg);
          int hh = (int)h, mm = (int)((h - hh) * 60.0 + 0.5);
          if (mm >= 60) { mm -= 60; hh = hh % 12 + 1; }
          f.set(line, "Polaris at %2d:%02d", hh, mm);
        } else {
          f.set(line, "Polaris       ---");
        }
        break;
      case 5:
        if (ctx.state.polar_aligned) f.set(line, "State: ALIGNED");
        else if (ok_)                f.set(line, "ALT OK. AZ: Polaris");
        else                         f.set(line, "State: adjust");
        break;
      case INFO_LINES + CONFIRM:
        f.set(line, "%c%s", cursor_.mark(CONFIRM, blink),
              ctx.state.polar_aligned ? "Clear aligned" : (ok_ ? "Confirm aligned" : "(Confirm: wait OK)"));
        break;
      default:
        f.set(line, "%cBack", cursor_.mark(BACK, blink));
        break;
    }
  }
}

/************************************/
// Settings
/************************************/
void SettingsScreen::onRotate(int delta, UiContext& ctx) {
  if (editing_) {
    float& tol = ctx.state.guide_tolerance_deg;
    tol += TOL_STEP * delta;
    if (tol < 0.1f) tol = 0.1f;
    if (tol > 20.0f) tol = 20.0f;
    return;
  }
  cursor_.move(delta);
  confirm_reset_ = false;
}

void SettingsScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  UiState& st = ctx.state;
  if (editing_) { editing_ = false; return; }
  switch (cursor_.selected()) {
    case SET_RA_DUTY:                       // RRcmd found on RA Speed by following a star
      st.ra_duty_at_sidereal = st.ra_speed_cmd;
      st.settings_save_request = true;
      break;
    case LOCATION:  mgr.push(&location_, ctx); break;
    case TIME:      mgr.push(&time_, ctx); break;
    case MOTOR_DIR: st.motor_reverse = !st.motor_reverse; break;
    case GUIDE_TOL: editing_ = true; break;
    case SAVE:
      st.settings_save_request = true;
      break;
    case RESET:
      if (!confirm_reset_) { confirm_reset_ = true; return; }
      st.settings_reset_request = true;
      break;
    case BACK:
      mgr.pop();
      break;
  }
  confirm_reset_ = false;
}

static const char* settingsStatusText(int s) {
  switch (s) {
    case Telemetry::SETTINGS_LOADED:      return "loaded";
    case Telemetry::SETTINGS_SAVED:       return "saved";
    case Telemetry::SETTINGS_SAVE_FAILED: return "SAVE FAILED";
    default:                              return "defaults";
  }
}

void SettingsScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const UiState& st = ctx.state;
  f.set(1, "*%s", title_);
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int line = Frame::CONTENT_FIRST + row;
    const int i = first + row - INFO_LINES;     // item index (negative: info line)
    const char m = i >= 0 ? cursor_.mark(i, blink) : ' ';
    switch (first + row) {
      case 0: f.set(line, "Flash: %s", settingsStatusText(ctx.tel.settings_status)); break;
      case 1: f.set(line, "RAduty %+9.1f PWM", st.ra_duty_at_sidereal); break;
      default:
        switch (i) {
          case SET_RA_DUTY: f.set(line, "%cRAduty = RRcmd", m); break;
          case LOCATION:    f.set(line, "%cLocation >", m); break;
          case TIME:        f.set(line, "%cTime >", m); break;
          case MOTOR_DIR:   f.set(line, "%cMotor dir %s", m, st.motor_reverse ? "Reverse" : "Normal"); break;
          case GUIDE_TOL:   f.set(line, "%cGuide tol %5.2f deg", editing_ ? blink : m, st.guide_tolerance_deg); break;
          case SAVE:        f.set(line, "%cSave", m); break;
          case RESET:       f.set(line, "%c%s", m, confirm_reset_ ? "Reset? click again" : "Reset defaults"); break;
          default:          f.set(line, "%cBack", m); break;
        }
    }
  }
}

/************************************/
// Location (manual position)
/************************************/
static const double LAT_STEPS[] = {10, 1, 0.1, 0.01, 0.001, 0.0001};
static const double LON_STEPS[] = {100, 10, 1, 0.1, 0.01, 0.001, 0.0001};
static const double ALT_STEPS[] = {1000, 100, 10, 1};

int LocationScreen::stepCount() const {
  switch (cursor_.selected()) {
    case LAT: return 6;
    case LON: return 7;
    case ALT: return 4;
    default:  return 0;
  }
}

double LocationScreen::step() const {
  switch (cursor_.selected()) {
    case LAT: return LAT_STEPS[step_];
    case LON: return LON_STEPS[step_];
    case ALT: return ALT_STEPS[step_];
    default:  return 0.0;
  }
}

static double clampD(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

void LocationScreen::onRotate(int delta, UiContext& ctx) {
  if (!editing_) { cursor_.move(delta); return; }
  UiState& st = ctx.state;
  const double d = step() * delta;
  switch (cursor_.selected()) {
    case LAT: st.manual_lat_deg = clampD(st.manual_lat_deg + d, -90.0, 90.0); break;
    case LON: st.manual_lon_deg = clampD(st.manual_lon_deg + d, -180.0, 180.0); break;
    case ALT: st.manual_alt_m = (float)clampD(st.manual_alt_m + d, -1000.0, 9999.0); break;
    default: break;
  }
  st.manual_set = true;
}

void LocationScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  UiState& st = ctx.state;
  const Telemetry& t = ctx.tel;
  if (editing_) {                            // next digit, then done
    if (++step_ >= stepCount()) editing_ = false;
    return;
  }
  switch (cursor_.selected()) {
    case MODE:
      st.location_mode = st.location_mode == loc::MODE_AUTO ? loc::MODE_MANUAL : loc::MODE_AUTO;
      break;
    case LAT: case LON: case ALT:
      editing_ = true;
      step_ = 0;
      break;
    case FROM_GPS:
      if (t.fixquality > 0) {
        st.manual_lat_deg = t.latitude;
        st.manual_lon_deg = t.longitude;
        st.manual_alt_m = t.altitude_m;
        st.manual_set = true;
      }
      break;
    case SAVE: st.settings_save_request = true; break;
    default:   mgr.pop(); break;
  }
}

void LocationScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const UiState& st = ctx.state;
  if (editing_) f.set(1, "*%s step %g", cursor_.selected() == LAT ? "Lat" :
                       (cursor_.selected() == LON ? "Lon" : "Alt"), step());
  else          f.set(1, "*Location");
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int line = Frame::CONTENT_FIRST + row;
    const int i = first + row - INFO_LINES;
    const char m = i >= 0 ? (editing_ && i == cursor_.selected() ? '*' : cursor_.mark(i, blink)) : ' ';
    switch (i) {
      case -1:  f.set(line, "In use: %s", loc::sourceText(ctx.tel.loc_source)); break;
      case MODE: f.set(line, "%cMode %s", m, st.location_mode == loc::MODE_MANUAL ? "Manual" : "Auto"); break;
      case LAT: f.set(line, "%cLat %+9.4f deg", m, st.manual_lat_deg); break;
      case LON: f.set(line, "%cLon %+9.4f deg", m, st.manual_lon_deg); break;
      case ALT: f.set(line, "%cAlt %6.0f m", m, st.manual_alt_m); break;
      case FROM_GPS: f.set(line, "%cCopy GPS fix%s", m, ctx.tel.fixquality > 0 ? "" : " (none)"); break;
      case SAVE: f.set(line, "%cSave", m); break;
      default:  f.set(line, "%cBack", m); break;
    }
  }
}

/************************************/
// Time (manual clock, local time = UTC + offset)
/************************************/
void TimeScreen::onEnter(UiContext& ctx) {
  cursor_.reset();
  editing_ = false;
  set_ = false;
  if (ctx.tel.time_source == loc::TIME_NONE) return;        // keep the last entry
  astro::UtcTime u;
  if (!astro::utcFromJd(ctx.tel.jd_ut + ctx.state.utc_offset_min / 1440.0, u)) return;
  v_[0] = u.year; v_[1] = u.month; v_[2] = u.day;
  v_[3] = u.hour; v_[4] = u.minute; v_[5] = (int)u.second;
}

void TimeScreen::onRotate(int delta, UiContext& ctx) {
  if (!editing_) { cursor_.move(delta); return; }
  const int i = cursor_.selected();
  if (i == UTC_OFFSET) {
    int& off = ctx.state.utc_offset_min;
    off += OFFSET_STEP_MIN * delta;
    if (off < -12 * 60) off = -12 * 60;
    if (off > 14 * 60) off = 14 * 60;
    return;
  }
  static const int lo[6] = {2000, 1, 1, 0, 0, 0};
  static const int hi[6] = {2099, 12, 31, 23, 59, 59};
  int v = v_[i] + delta;
  const int span = hi[i] - lo[i] + 1;
  if (i == YEAR) v = v < lo[i] ? lo[i] : (v > hi[i] ? hi[i] : v);
  else v = lo[i] + ((v - lo[i]) % span + span) % span;      // wraps like a clock
  v_[i] = v;
}

void TimeScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  if (editing_) { editing_ = false; return; }
  switch (cursor_.selected()) {
    case SET: {
      if (v_[DAY] > astro::daysInMonth(v_[YEAR], v_[MONTH])) v_[DAY] = astro::daysInMonth(v_[YEAR], v_[MONTH]);
      const astro::UtcTime local{v_[0], v_[1], v_[2], v_[3], v_[4], (double)v_[5]};
      UiState& st = ctx.state;
      st.manual_clock_jd = astro::julianDate(local) - st.utc_offset_min / 1440.0;
      st.manual_clock_ms = ctx.tel.now_ms;
      st.manual_clock_valid = true;
      st.settings_save_request = true;         // the UTC offset is a setting
      set_ = true;
      break;
    }
    case BACK: mgr.pop(); break;
    default:   editing_ = true; set_ = false; break;
  }
}

void TimeScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const int off = ctx.state.utc_offset_min;
  const int aoff = off < 0 ? -off : off;
  f.set(1, "*Time (local)");
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int line = Frame::CONTENT_FIRST + row;
    const int i = first + row - INFO_LINES;
    const char m = i >= 0 ? (editing_ && i == cursor_.selected() ? '*' : cursor_.mark(i, blink)) : ' ';
    switch (i) {
      case -1: {
        const Telemetry& t = ctx.tel;
        astro::UtcTime u;
        if (t.time_source == loc::TIME_NONE || !astro::utcFromJd(t.jd_ut + off / 1440.0, u)) {
          f.set(line, "In use: ---");
          break;
        }
        f.set(line, "In use %s %02d:%02d:%02d", loc::timeSourceText(t.time_source),
              u.hour, u.minute, (int)u.second);
        break;
      }
      case YEAR:   f.set(line, "%cYear   %4d", m, v_[0]); break;
      case MONTH:  f.set(line, "%cMonth  %2d", m, v_[1]); break;
      case DAY:    f.set(line, "%cDay    %2d", m, v_[2]); break;
      case HOUR:   f.set(line, "%cHour   %2d", m, v_[3]); break;
      case MINUTE: f.set(line, "%cMinute %2d", m, v_[4]); break;
      case SECOND: f.set(line, "%cSecond %2d", m, v_[5]); break;
      case UTC_OFFSET: f.set(line, "%cUTC %c%02d:%02d", m, off < 0 ? '-' : '+', aoff / 60, aoff % 60); break;
      case SET:    f.set(line, "%cSet clock%s", m, set_ ? "  (set)" : ""); break;
      default:     f.set(line, "%cBack", m); break;
    }
  }
}

/************************************/
// Target
/************************************/
void TargetMenuScreen::onEnter(UiContext& ctx) {
  cursor_.reset();
  cursor_.move(ctx.state.track_target);   // start on the current target
}

void TargetMenuScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  const int sel = cursor_.selected();
  if (sel >= astro::TARGET_COUNT) { mgr.pop(); return; }   // Quit
  if (sel != ctx.state.track_target) {    // new target: start stopped, default rate (DES-TRK-01)
    ctx.state.tracking = false;
    ctx.state.track_mode = sel == (int)astro::Target::Moon ? UiState::TRACK_LUNAR : UiState::TRACK_SIDEREAL;
  }
  ctx.state.track_target = sel;
  mgr.push(&target_, ctx);
}

void TargetMenuScreen::render(Frame& f, const UiContext& ctx, char blink) {
  f.set(1, "*%s", title_);
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int i = first + row;
    if (i >= ITEMS) { f.blank(Frame::CONTENT_FIRST + row, Frame::CONTENT_FIRST + row); continue; }
    if (i >= astro::TARGET_COUNT) {
      f.set(Frame::CONTENT_FIRST + row, "%cQuit", cursor_.mark(i, blink));
      continue;
    }
    const bool trk = ctx.state.tracking && i == ctx.state.track_target;
    const char* name = astro::targetName(static_cast<astro::Target>(i));
    if (trk) f.set(Frame::CONTENT_FIRST + row, "%c%-8s TRACKING", cursor_.mark(i, blink), name);
    else     f.set(Frame::CONTENT_FIRST + row, "%c%s", cursor_.mark(i, blink), name);
  }
}

void TargetScreen::onRotate(int delta, UiContext& ctx) {
  if (editing_) ctx.state.track_offset += OFFSET_STEP * delta;
  else cursor_.move(delta);
}

void TargetScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  if (editing_) { editing_ = false; return; }
  if (cursor_.viewingInfo()) return;
  UiState& st = ctx.state;
  switch (cursor_.selected()) {
    case TRACK:  st.tracking = !st.tracking; break;
    case RATE:   st.track_mode = st.track_mode == UiState::TRACK_LUNAR ? UiState::TRACK_SIDEREAL
                                                                        : UiState::TRACK_LUNAR; break;
    case OFFSET: editing_ = true; break;
    default:     mgr.pop(); break;
  }
}

void TargetScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const Telemetry& t = ctx.tel;
  const UiState& st = ctx.state;
  const char* name = astro::targetName(static_cast<astro::Target>(st.track_target));
  const bool pending = t.target_index != st.track_target;   // chosen this frame, computed next
  const bool ok = !pending && t.target_valid;
  const bool guide = ok && t.scope_valid;
  double dra = 0.0, ddec = 0.0;
  if (guide) astro::guideOffsets(t.scope_ra_deg, t.scope_dec_deg, t.target_ra_deg, t.target_dec_deg, dra, ddec);
  const bool on = guide && astro::onTarget(dra, ddec, t.target_dec_deg, st.guide_tolerance_deg);
  if (pending)  f.set(1, "*%-7s computing...", name);
  else if (!ok) f.set(1, "*%-7s no loc/time", name);
  else if (on)  f.set(1, "*%-7s ON TARGET", name);
  else          f.set(1, "*%s", name);
  // Pointing from gravity (DES-PNT-02) assumes a polar-aligned mount: '?' until confirmed
  const char aligned = st.polar_aligned ? ' ' : '?';
  const int first = cursor_.first();
  for (int row = 0; row < Frame::CONTENT_LINES; row++) {
    const int line = Frame::CONTENT_FIRST + row;
    switch (first + row) {
      case 0:  // target, then where the tube points (T.: telescope)
        if (ok) f.set(line, "R.A.  %10.5f DEG", t.target_ra_deg);
        else    f.set(line, "R.A.         ---");
        break;
      case 1:
        if (ok) f.set(line, "DEC.  %10.5f DEG", t.target_dec_deg);
        else    f.set(line, "DEC.         ---");
        break;
      case 2:
        if (t.scope_valid) f.set(line, "T.R.A.%10.5f DEG%c", t.scope_ra_deg, aligned);
        else               f.set(line, "T.R.A.       ---");
        break;
      case 3:
        if (t.scope_valid) f.set(line, "T.DEC.%10.5f DEG%c", t.scope_dec_deg, aligned);
        else               f.set(line, "T.DEC.       ---");
        break;
      case 4:   // DES-PNT-04 : turn the tube by this much (RA in minutes of time)
        if (guide) f.set(line, "dRA  %+8.2f min %c", dra * 4.0, dra >= 0.0 ? 'E' : 'W');
        else       f.set(line, "dRA          ---");
        break;
      case 5:
        if (guide) f.set(line, "dDEC %+8.2f deg %c", ddec, ddec >= 0.0 ? 'N' : 'S');
        else       f.set(line, "dDEC         ---");
        break;
      case 6:
        if (ok) f.set(line, "ALT %+6.2f AZ %6.2f", t.target_alt_deg, t.target_az_deg);
        else    f.set(line, "ALT   --- AZ   ---");
        break;
      case INFO_LINES + TRACK: {
        double rate = 0.0;
        if (app::trackingRate(t, st, rate))
          f.set(line, "%cTrack %-3s %7.4f\"/s", cursor_.mark(TRACK, blink), st.tracking ? "ON" : "OFF", rate);
        else
          f.set(line, "%cTrack %-3s   ---", cursor_.mark(TRACK, blink), st.tracking ? "ON" : "OFF");
        break;
      }
      case INFO_LINES + RATE:
        f.set(line, "%cRate %s", cursor_.mark(RATE, blink),
              st.track_mode == UiState::TRACK_LUNAR ? "Lunar" : "Sidereal");
        break;
      case INFO_LINES + OFFSET:
        f.set(line, "%cOffset %+8.1f PWM", editing_ ? blink : cursor_.mark(OFFSET, blink), st.track_offset);
        break;
      default:
        f.set(line, "%cBack%s", cursor_.mark(BACK, blink),
              (st.tracking && !t.ra_calibrated) ? "   (RA uncal)" : "");
        break;
    }
  }
}
