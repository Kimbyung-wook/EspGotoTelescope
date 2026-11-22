// GPS time/position -> Moon position, recomputed once per period.
// Pure logic (no Arduino calls), so the host tests and the UI simulator use it as-is.
#pragma once
#include "astro.h"

namespace astro {

// Adafruit_GPS gives a 2-digit year (26 = 2026). Returns false when the GPS has
// not delivered a date yet (year/month/day are 0 before the first RMC sentence).
inline bool utcFromGps(int yy, int month, int day, int hour, int minute, int seconds, int ms,
                       UtcTime& out) {
  if (month < 1 || month > 12 || day < 1 || day > 31 || yy < 0 || yy > 99) return false;
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || seconds < 0 || seconds > 60) return false;
  out.year = 2000 + yy;
  out.month = month;
  out.day = day;
  out.hour = hour;
  out.minute = minute;
  out.second = seconds + ms / 1000.0;
  return true;
}

class MoonTracker {
 public:
  explicit MoonTracker(unsigned long period_ms = 1000) : period_ms_(period_ms) {}

  // Call every loop. Computes only when the period has passed, the GPS has a fix
  // and the date is valid. Returns true when a new state was computed.
  bool update(unsigned long now_ms, bool has_fix, const UtcTime* utc, const Observer& obs) {
    if (!has_fix || utc == nullptr) { valid_ = false; return false; }
    if (computed_once_ && (unsigned long)(now_ms - last_ms_) < period_ms_) return false;
    state_ = moonState(julianDate(*utc), obs);
    last_ms_ = now_ms;
    computed_once_ = true;
    valid_ = true;
    return true;
  }

  bool valid() const { return valid_; }
  const MoonState& state() const { return state_; }

 private:
  unsigned long period_ms_;
  unsigned long last_ms_ = 0;
  bool computed_once_ = false;
  bool valid_ = false;
  MoonState state_{};
};

}  // namespace astro
