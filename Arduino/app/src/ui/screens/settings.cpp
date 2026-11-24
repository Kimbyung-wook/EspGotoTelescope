#include "settings.h"
#include "../../astro/astro.h"
#include "../../location/location.h"

/************************************/
// Settings
/************************************/
void SettingsScreen::onRotate(int delta, UiContext& ctx) {
  if (editing_) {
    float& tol = ctx.state.cfg.guide_tolerance_deg;
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
      st.cfg.ra_duty_at_sidereal = st.ra_speed_cmd;
      st.settings_save_request = true;
      break;
    case LOCATION:  mgr.push(&location_, ctx); break;
    case TIME:      mgr.push(&time_, ctx); break;
    case MOTOR_DIR: st.cfg.motor_reverse = !st.cfg.motor_reverse; break;
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
  renderPage([&](int line, int n) {
    const int i = itemAt(n);
    const char m = markAt(i, blink);
    switch (n) {
      case 0: f.set(line, "Flash: %s", settingsStatusText(ctx.tel.settings_status)); break;
      case 1: f.set(line, "RAduty %+9.1f PWM", st.cfg.ra_duty_at_sidereal); break;
      default:
        switch (i) {
          case SET_RA_DUTY: f.set(line, "%cRAduty = RRcmd", m); break;
          case LOCATION:    f.set(line, "%cLocation >", m); break;
          case TIME:        f.set(line, "%cTime >", m); break;
          case MOTOR_DIR:   f.set(line, "%cMotor dir %s", m, st.cfg.motor_reverse ? "Reverse" : "Normal"); break;
          case GUIDE_TOL:   f.set(line, "%cGuide tol %5.2f deg", editing_ ? blink : m, st.cfg.guide_tolerance_deg); break;
          case SAVE:        f.set(line, "%cSave", m); break;
          case RESET:       f.set(line, "%c%s", m, confirm_reset_ ? "Reset? click again" : "Reset defaults"); break;
          default:          f.set(line, "%cBack", m); break;
        }
    }
  });
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
    case LAT: st.cfg.manual_lat_deg = clampD(st.cfg.manual_lat_deg + d, -90.0, 90.0); break;
    case LON: st.cfg.manual_lon_deg = clampD(st.cfg.manual_lon_deg + d, -180.0, 180.0); break;
    case ALT: st.cfg.manual_alt_m = (float)clampD(st.cfg.manual_alt_m + d, -1000.0, 9999.0); break;
    default: break;
  }
  st.cfg.manual_set = true;
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
      st.cfg.location_mode = st.cfg.location_mode == loc::MODE_AUTO ? loc::MODE_MANUAL : loc::MODE_AUTO;
      break;
    case LAT: case LON: case ALT:
      editing_ = true;
      step_ = 0;
      break;
    case FROM_GPS:
      if (t.gps.fixquality > 0) {
        st.cfg.manual_lat_deg = t.gps.latitude;
        st.cfg.manual_lon_deg = t.gps.longitude;
        st.cfg.manual_alt_m = t.gps.altitude_m;
        st.cfg.manual_set = true;
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
  renderPage([&](int line, int n) {
    const int i = itemAt(n);
    const char m = editMarkAt(i, blink, editing_);
    switch (i) {
      case -1:  f.set(line, "In use: %s", loc::sourceText(ctx.tel.obs.loc_source)); break;
      case MODE: f.set(line, "%cMode %s", m, st.cfg.location_mode == loc::MODE_MANUAL ? "Manual" : "Auto"); break;
      case LAT: f.set(line, "%cLat %+9.4f deg", m, st.cfg.manual_lat_deg); break;
      case LON: f.set(line, "%cLon %+9.4f deg", m, st.cfg.manual_lon_deg); break;
      case ALT: f.set(line, "%cAlt %6.0f m", m, st.cfg.manual_alt_m); break;
      case FROM_GPS: f.set(line, "%cCopy GPS fix%s", m, ctx.tel.gps.fixquality > 0 ? "" : " (none)"); break;
      case SAVE: f.set(line, "%cSave", m); break;
      default:  f.set(line, "%cBack", m); break;
    }
  });
}

/************************************/
// Time (manual clock, local time = UTC + offset)
/************************************/
void TimeScreen::onEnter(UiContext& ctx) {
  cursor_.reset();
  editing_ = false;
  set_ = false;
  if (ctx.tel.obs.time_source == loc::TIME_NONE) return;        // keep the last entry
  astro::UtcTime u;
  if (!astro::utcFromJd(ctx.tel.obs.jd_ut + ctx.state.cfg.utc_offset_min / 1440.0, u)) return;
  v_[0] = u.year; v_[1] = u.month; v_[2] = u.day;
  v_[3] = u.hour; v_[4] = u.minute; v_[5] = (int)u.second;
}

void TimeScreen::onRotate(int delta, UiContext& ctx) {
  if (!editing_) { cursor_.move(delta); return; }
  const int i = cursor_.selected();
  if (i == UTC_OFFSET) {
    int off = ctx.state.cfg.utc_offset_min + OFFSET_STEP_MIN * delta;
    if (off < -12 * 60) off = -12 * 60;
    if (off > 14 * 60) off = 14 * 60;
    ctx.state.cfg.utc_offset_min = (int16_t)off;
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
      st.manual_clock_jd = astro::julianDate(local) - st.cfg.utc_offset_min / 1440.0;
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
  const int off = ctx.state.cfg.utc_offset_min;
  const int aoff = off < 0 ? -off : off;
  f.set(1, "*Time (local)");
  renderPage([&](int line, int n) {
    const int i = itemAt(n);
    const char m = editMarkAt(i, blink, editing_);
    switch (i) {
      case -1: {
        const Telemetry& t = ctx.tel;
        astro::UtcTime u;
        if (t.obs.time_source == loc::TIME_NONE || !astro::utcFromJd(t.obs.jd_ut + off / 1440.0, u)) {
          f.set(line, "In use: ---");
          break;
        }
        f.set(line, "In use %s %02d:%02d:%02d", loc::timeSourceText(t.obs.time_source),
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
  });
}
