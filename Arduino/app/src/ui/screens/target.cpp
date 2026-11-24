#include "target.h"
#include "../../app/tracking.h"
#include "../../astro/astro.h"

void TargetMenuScreen::onEnter(UiContext& ctx) {
  cursor_.reset();
  cursor_.move(ctx.state.cfg.track_target);   // start on the current target
}

void TargetMenuScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  const int sel = cursor_.selected();
  if (sel >= astro::TARGET_COUNT) { mgr.pop(); return; }   // Quit
  if (sel != ctx.state.cfg.track_target) {    // new target: start stopped, default rate (DES-TRK-01)
    ctx.state.tracking = false;
    ctx.state.cfg.track_mode = sel == (int)astro::Target::Moon ? UiState::TRACK_LUNAR : UiState::TRACK_SIDEREAL;
  }
  ctx.state.cfg.track_target = sel;
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
    const bool trk = ctx.state.tracking && i == ctx.state.cfg.track_target;
    const char* name = astro::targetName(static_cast<astro::Target>(i));
    if (trk) f.set(Frame::CONTENT_FIRST + row, "%c%-8s TRACKING", cursor_.mark(i, blink), name);
    else     f.set(Frame::CONTENT_FIRST + row, "%c%s", cursor_.mark(i, blink), name);
  }
}

void TargetScreen::onRotate(int delta, UiContext& ctx) {
  if (editing_) ctx.state.cfg.track_offset += OFFSET_STEP * delta;
  else cursor_.move(delta);
}

void TargetScreen::onClick(ScreenManager& mgr, UiContext& ctx) {
  if (editing_) { editing_ = false; return; }
  if (cursor_.viewingInfo()) return;
  UiState& st = ctx.state;
  switch (cursor_.selected()) {
    case TRACK:  st.tracking = !st.tracking; break;
    case RATE:   st.cfg.track_mode = st.cfg.track_mode == UiState::TRACK_LUNAR ? UiState::TRACK_SIDEREAL
                                                                        : UiState::TRACK_LUNAR; break;
    case OFFSET: editing_ = true; break;
    default:     mgr.pop(); break;
  }
}

void TargetScreen::render(Frame& f, const UiContext& ctx, char blink) {
  const Telemetry& t = ctx.tel;
  const UiState& st = ctx.state;
  const char* name = astro::targetName(static_cast<astro::Target>(st.cfg.track_target));
  const bool pending = t.target_index != st.cfg.track_target;   // chosen this frame, computed next
  const bool ok = !pending && t.target.valid;
  const bool guide = ok && t.scope.valid;
  double dra = 0.0, ddec = 0.0;
  if (guide) astro::guideOffsets(t.scope.ra_deg, t.scope.dec_deg, t.target.ra_deg, t.target.dec_deg, dra, ddec);
  const bool on = guide && astro::onTarget(dra, ddec, t.target.dec_deg, st.cfg.guide_tolerance_deg);
  if (pending)  f.set(1, "*%-7s computing...", name);
  else if (!ok) f.set(1, "*%-7s no loc/time", name);
  else if (on)  f.set(1, "*%-7s ON TARGET", name);
  else          f.set(1, "*%s", name);
  // Pointing from gravity (DES-PNT-02) assumes a polar-aligned mount: '?' until confirmed
  const char aligned = st.polar_aligned ? ' ' : '?';
  renderPage([&](int line, int n) {
    switch (n) {
      case 0:  // target, then where the tube points (T.: telescope)
        if (ok) f.set(line, "R.A.  %10.5f DEG", t.target.ra_deg);
        else    f.set(line, "R.A.         ---");
        break;
      case 1:
        if (ok) f.set(line, "DEC.  %10.5f DEG", t.target.dec_deg);
        else    f.set(line, "DEC.         ---");
        break;
      case 2:
        if (t.scope.valid) f.set(line, "T.R.A.%10.5f DEG%c", t.scope.ra_deg, aligned);
        else               f.set(line, "T.R.A.       ---");
        break;
      case 3:
        if (t.scope.valid) f.set(line, "T.DEC.%10.5f DEG%c", t.scope.dec_deg, aligned);
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
        if (ok) f.set(line, "ALT %+6.2f AZ %6.2f", t.target.alt_deg, t.target.az_deg);
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
              st.cfg.track_mode == UiState::TRACK_LUNAR ? "Lunar" : "Sidereal");
        break;
      case INFO_LINES + OFFSET:
        f.set(line, "%cOffset %+8.1f PWM", editing_ ? blink : cursor_.mark(OFFSET, blink), st.cfg.track_offset);
        break;
      default:
        f.set(line, "%cBack%s", cursor_.mark(BACK, blink),
              (st.tracking && !t.ra_calibrated) ? "   (RA uncal)" : "");
        break;
    }
  });
}
