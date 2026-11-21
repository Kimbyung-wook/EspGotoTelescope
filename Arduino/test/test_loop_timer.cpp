#include "test_main.h"
#include "../app/src/util/loop_timer.h"

TEST(loop_timer_fires_once_per_period) {
  LoopTimer t(100000); t.start(0);
  CHECK(!t.due(50000));
  CHECK(t.due(100000));
  CHECK(!t.due(150000));
  CHECK(t.due(200500));               // small lateness keeps the original grid
  CHECK(!t.due(299000));
  CHECK(t.due(300000));
  CHECK(t.overruns() == 0);
}

TEST(loop_timer_handles_micros_wrap) {
  const unsigned long near_max = (unsigned long)-1 - 30000;  // 30 ms before wrap
  LoopTimer t(100000); t.start(near_max);
  CHECK(!t.due(near_max + 50000));    // wrapped, still early
  CHECK(t.due(near_max + 100000));
  CHECK(!t.due(near_max + 150000));
}

TEST(loop_timer_skips_ahead_after_overrun) {
  LoopTimer t(100000); t.start(0);
  CHECK(t.due(350000));               // 2.5 periods late
  CHECK(t.overruns() == 1);
  CHECK(!t.due(400000));              // no burst of catch-up iterations
  CHECK(t.due(450000));
}
