#include "location.h"
#include <math.h>
#include "../util/angles.h"

namespace loc {

const char* sourceText(int src) {
  switch (src) {
    case SRC_GPS:    return "GPS";
    case SRC_LAST:   return "LAST";
    case SRC_MANUAL: return "MAN";
    default:         return "---";
  }
}

const char* timeSourceText(int src) {
  switch (src) {
    case TIME_GPS:    return "GPS";
    case TIME_MANUAL: return "MAN";
    case TIME_PHONE:  return "PHN";
    default:          return "---";
  }
}

int resolvePosition(int mode, bool gps_fix, const Position& gps,
                    bool last_valid, const Position& last,
                    bool manual_set, const Position& manual, Position& out) {
  if (mode == MODE_MANUAL) {
    if (!manual_set) return SRC_NONE;
    out = manual;
    return SRC_MANUAL;
  }
  if (gps_fix)    { out = gps;    return SRC_GPS; }
  if (last_valid) { out = last;   return SRC_LAST; }
  if (manual_set) { out = manual; return SRC_MANUAL; }
  return SRC_NONE;
}

double distanceKm(const Position& a, const Position& b) {
  const double D2R = util::kDegToRad;
  const double R = 6371.0;
  const double p1 = a.lat_deg * D2R, p2 = b.lat_deg * D2R;
  const double dp = p2 - p1, dl = (b.lon_deg - a.lon_deg) * D2R;
  const double h = sin(dp / 2) * sin(dp / 2) + cos(p1) * cos(p2) * sin(dl / 2) * sin(dl / 2);
  return 2.0 * R * asin(sqrt(h < 1.0 ? h : 1.0));
}

bool LastGpsRecorder::update(unsigned long now_ms, int fixquality, int satellites, float hdop,
                             const Position& gps, bool last_valid, const Position& last) {
  if (done_) return false;
  const bool good = fixquality > 0 && satellites >= MIN_SATELLITES && hdop > 0.0f && hdop <= MAX_HDOP;
  if (!good) { holding_ = false; return false; }
  if (!holding_) { holding_ = true; since_ms_ = now_ms; return false; }
  if ((unsigned long)(now_ms - since_ms_) < HOLD_MS) return false;
  done_ = true;                    // at most one write per power-on
  return !last_valid || distanceKm(gps, last) > MOVE_KM;
}

} // namespace loc
