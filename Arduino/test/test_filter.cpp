#include "test_main.h"
#include "../app/src/estimation/filter.h"

// 1st-order step response after t = tau : LPF -> 1 - e^-1, HPF -> e^-1
TEST(lpf_step_response_matches_first_order) {
  LPF lpf; lpf.initialize(1.0f, 0.1f);
  lpf.update(0.0f, 0.1f);
  float y = 0;
  for (int k = 0; k < 10; k++) y = lpf.update(1.0f, 0.1f);
  CHECK_NEAR(y, 1.0 - std::exp(-1.0), 1e-3);
}

TEST(lpf_starts_from_first_input) {
  LPF lpf; lpf.initialize(1.0f, 0.1f);
  CHECK_NEAR(lpf.update(5.0f, 0.1f), 5.0, 1e-6);
}

TEST(lpf_larger_tau_filters_more) {
  LPF fast; fast.initialize(0.5f, 0.1f); fast.update(0.0f, 0.1f);
  LPF slow; slow.initialize(2.0f, 0.1f); slow.update(0.0f, 0.1f);
  CHECK(slow.update(1.0f, 0.1f) < fast.update(1.0f, 0.1f));
}

TEST(hpf_step_response_matches_first_order) {
  HPF hpf; hpf.initialize(1.0f, 0.1f);
  hpf.update(0.0f, 0.1f);
  float y = 0;
  for (int k = 0; k < 10; k++) y = hpf.update(1.0f, 0.1f);
  CHECK_NEAR(y, std::exp(-1.0), 1e-3);
}

TEST(hpf_no_kick_at_start) {
  HPF hpf; hpf.initialize(1.0f, 0.1f);
  CHECK_NEAR(hpf.update(5.0f, 0.1f), 0.0, 1e-6);
}

TEST(complementary_filter_tracks_constant_angle) {
  ComplementaryFilter cf; cf.initialize(1.0f, 0.1f);
  float y = 0;
  for (int k = 0; k < 200; k++) y = cf.update(10.0f, 0.0f, 0.1f);
  CHECK_NEAR(y, 10.0, 1e-3);
}
