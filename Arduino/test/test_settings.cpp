#include "test_main.h"
#include "../app/src/config/settings.h"
#include "../app/src/config/settings_binding.h"

using namespace config;

namespace {
Settings sample() {
  Settings s = defaults();
  s.gyro_bias[0] = 0.125f; s.gyro_bias[1] = -0.5f; s.gyro_bias[2] = 0.0625f;
  s.accel_bias[2] = 0.2f;
  s.accel_scale[0] = 1.01f;
  s.mag_offset[1] = -12.5f;
  s.mag_scale[2] = 0.97f;
  s.declination_deg = -8.5f;
  s.location_mode = LOCATION_MANUAL;
  s.manual_lat_deg = 37.5665123; s.manual_lon_deg = 126.9780456; s.manual_alt_m = 38.0f;
  s.last_gps_valid = 1; s.last_lat_deg = -33.8688; s.last_lon_deg = 151.2093; s.last_alt_m = 58.0f;
  s.last_year = 2026; s.last_month = 11; s.last_day = 24;
  s.utc_offset_min = 540;
  s.ra_duty_at_sidereal = 187.5f;
  s.motor_reverse = 1;
  s.track_offset = -3.0f;
  s.track_target = 4;
  s.guide_tolerance_deg = 0.5f;
  return s;
}

bool same(const Settings& a, const Settings& b) {
  uint8_t x[STORE_SIZE], y[STORE_SIZE];
  const size_t n = encode(a, x, sizeof(x));
  return n == encode(b, y, sizeof(y)) && std::memcmp(x, y, n) == 0;
}
}

TEST(settings_image_fits_the_reserved_flash) {
  CHECK(encodedSize() <= STORE_SIZE);
  CHECK(encodedSize() > HEADER_SIZE + 100);
}

TEST(settings_round_trip) {
  const Settings s = sample();
  uint8_t img[STORE_SIZE];
  const size_t n = encode(s, img, sizeof(img));
  CHECK(n == encodedSize());
  Settings back;
  CHECK(decode(img, n, back) == LoadResult::Loaded);
  CHECK(same(s, back));
  CHECK(back.manual_lat_deg == 37.5665123);          // doubles keep every bit
  CHECK(back.utc_offset_min == 540);
  CHECK(back.track_target == 4);
}

TEST(settings_blank_flash_gives_defaults) {
  RamSettingsStore store;                             // erased: all 0xFF
  Settings s = sample();
  CHECK(load(store, s) == LoadResult::Blank);
  CHECK(same(s, defaults()));
  CHECK(s.accel_scale[1] == 1.0f && s.mag_scale[0] == 1.0f);
  CHECK(s.location_mode == LOCATION_AUTO);
}

TEST(settings_corruption_is_detected) {
  uint8_t img[STORE_SIZE];
  const size_t n = encode(sample(), img, sizeof(img));
  Settings s;
  img[HEADER_SIZE + 5] ^= 0x01;                       // one payload bit
  CHECK(decode(img, n, s) == LoadResult::Corrupt);
  CHECK(same(s, defaults()));
  img[HEADER_SIZE + 5] ^= 0x01;
  img[0] = 'X';                                       // magic
  CHECK(decode(img, n, s) == LoadResult::Corrupt);
  img[0] = 'S';
  CHECK(decode(img, n, s) == LoadResult::Loaded);
  img[5] = 0xFF;                                      // version from the future
  CHECK(decode(img, n, s) == LoadResult::Newer);
}

TEST(settings_older_shorter_payload_keeps_new_fields_default) {
  // An image whose payload ends before guide_tolerance_deg (last 4 bytes)
  uint8_t img[STORE_SIZE];
  const size_t n = encode(sample(), img, sizeof(img));
  const uint16_t short_size = (uint16_t)(n - HEADER_SIZE - 4);
  img[6] = (uint8_t)short_size; img[7] = (uint8_t)(short_size >> 8);
  const uint32_t crc = crc32(img + HEADER_SIZE, short_size);
  for (int i = 0; i < 4; i++) img[8 + i] = (uint8_t)(crc >> (8 * i));
  Settings s;
  CHECK(decode(img, n, s) == LoadResult::Loaded);
  CHECK(s.ra_duty_at_sidereal == 187.5f);             // present in the old payload
  CHECK(s.guide_tolerance_deg == defaults().guide_tolerance_deg);
}

TEST(settings_out_of_range_values_fall_back) {
  Settings s = sample();
  s.accel_scale[0] = 0.0f;                            // would divide by zero
  s.manual_lat_deg = 123.0;
  s.gyro_bias[1] = std::nanf("");
  s.location_mode = 7;
  CHECK(sanitize(s) == 4);
  CHECK(s.accel_scale[0] == 1.0f);
  CHECK(s.manual_lat_deg == 0.0);
  CHECK(s.gyro_bias[1] == 0.0f);
  CHECK(s.location_mode == LOCATION_AUTO);
  CHECK(s.gyro_bias[0] == 0.125f);                    // valid values stay
}

TEST(settings_crc32_known_value) {
  const uint8_t text[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  CHECK(crc32(text, sizeof(text)) == 0xCBF43926u);    // standard CRC-32 check value
}

TEST(settings_save_skips_identical_image) {
  RamSettingsStore store;
  const Settings s = sample();
  CHECK(save(store, s) == SaveResult::Saved);
  CHECK(store.writes == 1);
  CHECK(save(store, s) == SaveResult::Unchanged);
  CHECK(store.writes == 1);
  Settings back;
  CHECK(load(store, back) == LoadResult::Loaded);
  CHECK(same(s, back));
  store.fail_writes = true;
  Settings t = s; t.track_offset = 9.0f;
  CHECK(save(store, t) == SaveResult::Failed);
}

TEST(settings_ui_requests_save_and_reset) {
  RamSettingsStore store;
  Settings s = defaults();
  UiContext ctx;
  applyToUi(s, ctx.state);
  CHECK(!serviceRequests(store, s, ctx));             // nothing asked
  CHECK(store.writes == 0);

  ctx.state.gyro_bias[2] = 0.25f;
  ctx.state.ra_duty_at_sidereal = 150.0f;
  ctx.state.track_offset = 2.0f;
  ctx.state.settings_save_request = true;
  CHECK(serviceRequests(store, s, ctx));
  CHECK(!ctx.state.settings_save_request);
  CHECK(ctx.tel.settings_status == Telemetry::SETTINGS_SAVED);

  Settings back;                                      // power cycle
  CHECK(load(store, back) == LoadResult::Loaded);
  UiContext fresh;
  applyToUi(back, fresh.state);
  CHECK(fresh.state.gyro_bias[2] == 0.25f);
  CHECK(fresh.state.ra_duty_at_sidereal == 150.0f);
  CHECK(fresh.state.track_offset == 2.0f);

  ctx.state.settings_reset_request = true;            // Settings > Reset defaults
  CHECK(serviceRequests(store, s, ctx));
  CHECK(ctx.state.gyro_bias[2] == 0.0f);
  CHECK(ctx.state.track_offset == 0.0f);
  CHECK(load(store, back) == LoadResult::Loaded);
  CHECK(same(back, defaults()));

  store.fail_writes = true;
  ctx.state.track_offset = 5.0f;
  ctx.state.settings_save_request = true;
  CHECK(!serviceRequests(store, s, ctx));
  CHECK(ctx.tel.settings_status == Telemetry::SETTINGS_SAVE_FAILED);
}

TEST(settings_load_text_fits_the_screen) {
  const LoadResult all[] = {LoadResult::Loaded, LoadResult::Blank, LoadResult::Corrupt,
                            LoadResult::Newer, LoadResult::ReadFailed};
  for (LoadResult r : all) CHECK(std::strlen(loadText(r)) <= 21);
}
