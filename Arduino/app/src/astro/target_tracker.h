// Time/position in use -> target (Moon or planet) position, recomputed once per period.
// Pure logic (no Arduino calls), so the host tests and the UI simulator use it as-is.
#pragma once
#include "astro.h"

namespace astro {

class TargetTracker {
 public:
  explicit TargetTracker(Target target = Target::Moon, unsigned long period_ms = 1000)
      : target_(target), period_ms_(period_ms) {}

  // A new target is computed on the next update(), without waiting for the period.
  void setTarget(Target t) {
    if (t == target_) return;
    target_ = t;
    computed_once_ = false;
    valid_ = false;
  }
  Target target() const { return target_; }

  // Call every loop with the time as a JD (UT); 'known' = position and time are known.
  // Computes only when the period has passed. Returns true when a new state was computed.
  bool updateJd(unsigned long now_ms, bool known, double jd_ut, const Observer& obs) {
    if (!known) { valid_ = false; return false; }
    if (computed_once_ && (unsigned long)(now_ms - last_ms_) < period_ms_) return false;
    state_ = targetState(target_, jd_ut, obs);
    last_ms_ = now_ms;
    computed_once_ = true;
    valid_ = true;
    return true;
  }

  bool valid() const { return valid_; }
  const TargetState& state() const { return state_; }

 private:
  Target target_;
  unsigned long period_ms_;
  unsigned long last_ms_ = 0;
  bool computed_once_ = false;
  bool valid_ = false;
  TargetState state_{};
};

}  // namespace astro
