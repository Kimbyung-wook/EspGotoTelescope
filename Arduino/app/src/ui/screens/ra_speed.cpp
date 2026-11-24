#include "ra_speed.h"
#include <math.h>

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
    f.set(3, " RRate %10.5fDPS", t.imu.corr.wx);
    f.set(4, " Exit");
  } else {
    f.set(1, "*%s", title_);
    f.set(2, "%cRRcmd %10.5fDEG", cursor_.mark(0, blink), ctx.state.ra_speed_cmd);
    f.set(3, "%cRRate %10.5fDPS", cursor_.mark(1, blink), t.imu.corr.wx);
    f.set(4, "%cQuit",            cursor_.mark(2, blink));
  }
  f.set(5, " ROL %10.5f  DEG", t.imu.roll_acc_deg);
  f.set(6, " LAT %c%10.6fDEG", t.obs.lat_deg >= 0.0 ? 'N' : 'S', fabs(t.obs.lat_deg));
  f.set(7, " PIT %10.5f  DEG", t.imu.pitch_acc_deg);
}
