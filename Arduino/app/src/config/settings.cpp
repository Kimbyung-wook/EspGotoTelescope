#include "settings.h"
#include <math.h>
#include <string.h>
#include "../board_config.h"

namespace config {
namespace {

// ---- little endian writer / reader (same bytes on the ESP8266 and the PC)
struct Writer {
  uint8_t* p;
  size_t n = 0;
  void u8(uint8_t v) { p[n++] = v; }
  void u16(uint16_t v) { u8((uint8_t)v); u8((uint8_t)(v >> 8)); }
  void u32(uint32_t v) { u16((uint16_t)v); u16((uint16_t)(v >> 16)); }
  void u64(uint64_t v) { u32((uint32_t)v); u32((uint32_t)(v >> 32)); }
  void f32(float v) { uint32_t b; memcpy(&b, &v, 4); u32(b); }
  void f64(double v) { uint64_t b; memcpy(&b, &v, 8); u64(b); }
  void i8(int8_t v) { u8((uint8_t)v); }
  void i16(int16_t v) { u16((uint16_t)v); }
  void b8(bool v) { u8(v ? 1 : 0); }
};

// Reads until the payload ends; after that every read leaves the value as it is
// (the caller filled in the defaults first).
struct Reader {
  const uint8_t* p;
  size_t size;
  size_t n = 0;
  bool have(size_t k) const { return n + k <= size; }
  uint64_t raw(size_t k) {
    uint64_t v = 0;
    for (size_t i = 0; i < k; i++) v |= (uint64_t)p[n + i] << (8 * i);
    n += k;
    return v;
  }
  void u8(uint8_t& v)   { if (have(1)) v = (uint8_t)raw(1); }
  void u16(uint16_t& v) { if (have(2)) v = (uint16_t)raw(2); }
  void i8(int8_t& v)    { if (have(1)) v = (int8_t)raw(1); }
  void i16(int16_t& v)  { if (have(2)) v = (int16_t)raw(2); }
  void f32(float& v)    { if (have(4)) { uint32_t b = (uint32_t)raw(4); memcpy(&v, &b, 4); } }
  void f64(double& v)   { if (have(8)) { uint64_t b = raw(8); memcpy(&v, &b, 8); } }
  void b8(bool& v)      { if (have(1)) v = raw(1) == 1; }   // 2..255: not a valid flag -> false
};

// The one list of fields, in stored order (append new fields at the end).
template <class Io, class S>
void fields(Io& io, S& s) {
  for (int i = 0; i < 3; i++) io.f32(s.gyro_bias[i]);
  for (int i = 0; i < 3; i++) io.f32(s.accel_bias[i]);
  for (int i = 0; i < 3; i++) io.f32(s.accel_scale[i]);
  for (int i = 0; i < 3; i++) io.f32(s.mag_offset[i]);
  for (int i = 0; i < 3; i++) io.f32(s.mag_scale[i]);
  io.f32(s.declination_deg);
  io.u8(s.location_mode);
  io.f64(s.manual_lat_deg);
  io.f64(s.manual_lon_deg);
  io.f32(s.manual_alt_m);
  io.b8(s.last_gps_valid);
  io.f64(s.last_lat_deg);
  io.f64(s.last_lon_deg);
  io.f32(s.last_alt_m);
  io.u16(s.last_year);
  io.u8(s.last_month);
  io.u8(s.last_day);
  io.i16(s.utc_offset_min);
  io.f32(s.ra_duty_at_sidereal);
  io.b8(s.motor_reverse);
  io.u8(s.track_mode);
  io.f32(s.track_offset);
  io.i8(s.track_target);
  io.f32(s.guide_tolerance_deg);
  io.b8(s.manual_set);
}

// Writer takes values, Reader takes references : small adapters for fields()
struct ConstWriter {
  Writer& w;
  void u8(uint8_t v) { w.u8(v); }
  void u16(uint16_t v) { w.u16(v); }
  void i8(int8_t v) { w.i8(v); }
  void i16(int16_t v) { w.i16(v); }
  void f32(float v) { w.f32(v); }
  void f64(double v) { w.f64(v); }
  void b8(bool v) { w.b8(v); }
};

// Counts the payload bytes without writing them
struct Counter {
  size_t n = 0;
  void u8(uint8_t)   { n += 1; }
  void u16(uint16_t) { n += 2; }
  void i8(int8_t)    { n += 1; }
  void i16(int16_t)  { n += 2; }
  void f32(float)    { n += 4; }
  void f64(double)   { n += 8; }
  void b8(bool)      { n += 1; }
};

size_t payloadSize() {
  const Settings d = defaults();
  Counter c;
  fields(c, d);
  return c.n;
}

bool finite(double v) { return !isnan(v) && !isinf(v); }

template <class T>
int fix(T& v, T def, double lo, double hi) {
  if (!finite((double)v) || (double)v < lo || (double)v > hi) { v = def; return 1; }
  return 0;
}

} // namespace

Settings defaults() {
  Settings s;
  memset(&s, 0, sizeof(s));
  for (int i = 0; i < 3; i++) {
    s.accel_scale[i] = 1.0f;
    s.mag_scale[i] = 1.0f;
  }
  s.location_mode = LOCATION_AUTO;
  s.last_gps_valid = false;
  s.utc_offset_min = 0;
  s.ra_duty_at_sidereal = RA_DUTY_AT_SIDEREAL;
  s.track_target = 1;              // Mercury
  s.guide_tolerance_deg = 1.0f;
  return s;
}

int sanitize(Settings& s) {
  const Settings d = defaults();
  int n = 0;
  for (int i = 0; i < 3; i++) {
    n += fix(s.gyro_bias[i], d.gyro_bias[i], -50.0, 50.0);
    n += fix(s.accel_bias[i], d.accel_bias[i], -5.0, 5.0);
    n += fix(s.accel_scale[i], d.accel_scale[i], 0.5, 2.0);
    n += fix(s.mag_offset[i], d.mag_offset[i], -500.0, 500.0);
    n += fix(s.mag_scale[i], d.mag_scale[i], 0.2, 5.0);
  }
  n += fix(s.declination_deg, d.declination_deg, -90.0, 90.0);
  if (s.location_mode > LOCATION_MANUAL) { s.location_mode = d.location_mode; n++; }
  n += fix(s.manual_lat_deg, d.manual_lat_deg, -90.0, 90.0);
  n += fix(s.manual_lon_deg, d.manual_lon_deg, -180.0, 180.0);
  n += fix(s.manual_alt_m, d.manual_alt_m, -1000.0, 10000.0);
  n += fix(s.last_lat_deg, d.last_lat_deg, -90.0, 90.0);
  n += fix(s.last_lon_deg, d.last_lon_deg, -180.0, 180.0);
  n += fix(s.last_alt_m, d.last_alt_m, -1000.0, 10000.0);
  n += fix(s.utc_offset_min, d.utc_offset_min, -12 * 60.0, 14 * 60.0);
  n += fix(s.ra_duty_at_sidereal, d.ra_duty_at_sidereal, -1023.0, 1023.0);
  if (s.track_mode > 1) { s.track_mode = 0; n++; }
  if (s.last_month > 12 || s.last_day > 31) { s.last_gps_valid = false; s.last_month = s.last_day = 0; n++; }
  n += fix(s.track_offset, d.track_offset, -1023.0, 1023.0);
  n += fix(s.track_target, d.track_target, 0.0, 15.0);
  n += fix(s.guide_tolerance_deg, d.guide_tolerance_deg, 0.05, 20.0);
  return n;
}

size_t encodedSize() { return HEADER_SIZE + payloadSize(); }

size_t encode(const Settings& s, uint8_t* out, size_t out_size) {
  const size_t total = encodedSize();
  if (out_size < total) return 0;
  Writer w{out + HEADER_SIZE};
  ConstWriter cw{w};
  fields(cw, s);
  Writer h{out};
  h.u32(SETTINGS_MAGIC);
  h.u16(SETTINGS_VERSION);
  h.u16((uint16_t)w.n);
  h.u32(crc32(out + HEADER_SIZE, w.n));
  return total;
}

LoadResult decode(const uint8_t* in, size_t in_size, Settings& s) {
  s = defaults();
  if (in_size < HEADER_SIZE) return LoadResult::Corrupt;
  bool blank = true;
  for (size_t i = 0; i < HEADER_SIZE; i++) blank = blank && in[i] == 0xFF;
  if (blank) return LoadResult::Blank;

  Reader h{in, HEADER_SIZE};
  uint8_t b[4] = {0, 0, 0, 0};
  for (int i = 0; i < 4; i++) h.u8(b[i]);
  const uint32_t magic = b[0] | (b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
  uint16_t version = 0, size = 0;
  h.u16(version);
  h.u16(size);
  uint8_t c[4] = {0, 0, 0, 0};
  for (int i = 0; i < 4; i++) h.u8(c[i]);
  const uint32_t crc = c[0] | (c[1] << 8) | ((uint32_t)c[2] << 16) | ((uint32_t)c[3] << 24);

  if (magic != SETTINGS_MAGIC) return LoadResult::Corrupt;
  if (version > SETTINGS_VERSION) return LoadResult::Newer;
  if (size == 0 || HEADER_SIZE + size > in_size) return LoadResult::Corrupt;
  if (crc32(in + HEADER_SIZE, size) != crc) return LoadResult::Corrupt;

  Reader r{in + HEADER_SIZE, size};
  fields(r, s);       // a shorter (older) payload leaves the later fields at default
  sanitize(s);
  return LoadResult::Loaded;
}

uint32_t crc32(const uint8_t* data, size_t n) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; i++) {
    crc ^= data[i];
    for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

const char* loadText(LoadResult r) {
  switch (r) {
    case LoadResult::Loaded:     return "6. Settings loaded";
    case LoadResult::Blank:      return "6. Settings new";
    case LoadResult::Corrupt:    return ">> Settings bad->def";
    case LoadResult::Newer:      return ">> Settings newer";
    case LoadResult::ReadFailed: return ">> Settings read err";
  }
  return "6. Settings ?";
}

LoadResult load(SettingsStore& store, Settings& s) {
  uint8_t buf[STORE_SIZE];
  if (!store.read(buf, STORE_SIZE)) {
    s = defaults();
    return LoadResult::ReadFailed;
  }
  return decode(buf, STORE_SIZE, s);
}

SaveResult save(SettingsStore& store, const Settings& s) {
  uint8_t img[STORE_SIZE];
  const size_t n = encode(s, img, sizeof(img));
  if (n == 0) return SaveResult::Failed;
  uint8_t cur[STORE_SIZE];
  if (store.read(cur, n) && memcmp(cur, img, n) == 0) return SaveResult::Unchanged;
  return store.write(img, n) ? SaveResult::Saved : SaveResult::Failed;
}

} // namespace config
