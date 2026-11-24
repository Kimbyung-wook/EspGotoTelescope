#include "polar_align.h"
#include <math.h>
#include "../../astro/astro.h"
#include "../../location/location.h"
#include "../../board_config.h"

void PolarAlignScreen::tick(UiContext& ctx) {
  ctx.state.motor_inhibit = true;            // DES-PA-01
  const Telemetry& t = ctx.tel;
  // DES-PA-04 : altitude only for now (azimuth needs the magnetometer, DES-MAG)
  const bool inside = t.obs.loc_source != loc::SRC_NONE && fabs(t.scope.tube_alt_deg - t.obs.lat_deg) <= PA_ALT_TOL_DEG;
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
  const bool have_lat = t.obs.loc_source != loc::SRC_NONE;
  const double dalt = t.scope.tube_alt_deg - t.obs.lat_deg;
  f.set(1, "*%s", title_);
  renderPage([&](int line, int n) {
    switch (n) {
      case 0: f.set(line, "Dec 90: tube // axis"); break;
      case 1:
        if (have_lat) f.set(line, "ALT%+7.2f LAT%+7.2f", t.scope.tube_alt_deg, t.obs.lat_deg);
        else          f.set(line, "ALT%+7.2f LAT    ---", t.scope.tube_alt_deg);
        break;
      case 2:
        if (!have_lat) f.set(line, "dALT   --- no loc");
        else f.set(line, "dALT %+6.2f %s", dalt,
                   fabs(dalt) <= PA_ALT_TOL_DEG ? "OK" : (dalt < 0.0 ? "UP" : "DOWN"));
        break;
      case 3: f.set(line, "dAZ    --- no mag"); break;
      case 4:
        if (t.scope.polaris_valid) {
          const double h = astro::polarisClockHours(t.scope.polaris_ha_deg);
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
  });
}
