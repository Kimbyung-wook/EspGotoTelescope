#pragma once
#ifndef _LOOP_TIMER_H_
#define _LOOP_TIMER_H_

/************************************/
// Fixed-rate loop timing (micros() wrap safe)
//   if (timer.due(micros())) { ...; }  or  while (!timer.due(micros())) yield();
// When the loop overruns by a full period or more, the timer skips ahead
// instead of running several late iterations back to back.
/************************************/
class LoopTimer {
public:
  explicit LoopTimer(unsigned long period_us) : period_(period_us) {}

  void start(unsigned long now_us) { next_ = now_us + period_; }

  // true once per period; schedules the next deadline
  bool due(unsigned long now_us) {
    long late = (long)(now_us - next_);
    if (late < 0) return false;
    if ((unsigned long)late >= period_) { next_ = now_us + period_; overruns_++; }
    else                                { next_ += period_; }
    return true;
  }

  unsigned long period()   const { return period_; }
  unsigned long overruns() const { return overruns_; }

private:
  unsigned long period_;
  unsigned long next_ = 0;
  unsigned long overruns_ = 0;
};

#endif // _LOOP_TIMER_H_
