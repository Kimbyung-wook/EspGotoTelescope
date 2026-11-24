#pragma once
#ifndef _CONFIG_SETTINGS_H_
#define _CONFIG_SETTINGS_H_
#include <stddef.h>
#include <stdint.h>

/************************************/
// Settings kept across power cycles (docs/REQUIREMENTS.md DES-CFG-01..06)
//
// Stored image : header (magic, version, payload size, CRC-32 of the payload)
//                + payload written field by field, little endian.
//   - A shorter payload (older version) is read up to its end; the fields it
//     does not have keep their defaults. So new fields go at the END only.
//   - Values out of range (or NaN) are replaced by their defaults on load.
//   - bool fields are stored as one byte, 0 or 1 (any other byte reads as false).
// Hardware independent : the flash storage is behind SettingsStore.
/************************************/
namespace config {

struct Settings {
  // Calibration (DES-CAL-03, DES-CAL-05, DES-MAG-01)
  float gyro_bias[3];        // [deg/s]
  float accel_bias[3];       // [m/s^2]
  float accel_scale[3];      // 1 = no correction
  float mag_offset[3];       // [uT] hard iron
  float mag_scale[3];        // soft iron, per axis
  float declination_deg;     // magnetic declination, east positive

  // Location and time (DES-LOC-01..04)
  uint8_t location_mode;     // LOCATION_AUTO / LOCATION_MANUAL
  double  manual_lat_deg, manual_lon_deg;   // signed (S/W negative)
  float   manual_alt_m;
  bool    last_gps_valid;    // last_* below hold a GPS fix
  double  last_lat_deg, last_lon_deg;
  float   last_alt_m;
  uint16_t last_year;        // date of that fix
  uint8_t  last_month, last_day;
  int16_t utc_offset_min;    // local time = UTC + this (e.g. +540 for UTC+9)

  // Motor and tracking (DES-MOT-01, DES-MOT-02, DES-TRK-01)
  float   ra_duty_at_sidereal;   // RRcmd that turns the RA axis at 15.041"/s, 0 = not set
  bool    motor_reverse;         // swap the motor direction
  uint8_t track_mode;            // DES-TRK-01 : 0 = sidereal, 1 = lunar
  float   track_offset;          // [PWM duty]
  int8_t  track_target;          // astro::Target index

  // Guidance (DES-PNT-04)
  float guide_tolerance_deg;

  // Added after the first layout (still version 1: older images leave it 0)
  bool    manual_set;            // manual_lat/lon/alt were entered (DES-LOC-01)
};

enum LocationMode : uint8_t { LOCATION_AUTO = 0, LOCATION_MANUAL = 1 };

enum class LoadResult {
  Loaded,         // valid image read
  Blank,          // erased flash (first boot) -> defaults
  Corrupt,        // wrong magic, size or CRC -> defaults
  Newer,          // written by a newer firmware with an unknown layout -> defaults
  ReadFailed      // the store could not be read -> defaults
};

enum class SaveResult { Saved, Unchanged, Failed };

static const uint32_t SETTINGS_MAGIC   = 0x4F544753u;  // "SGTO"
static const uint16_t SETTINGS_VERSION = 1;
static const size_t   HEADER_SIZE      = 12;           // magic 4, version 2, size 2, crc 4
static const size_t   STORE_SIZE       = 256;          // bytes reserved in flash

// Defaults : no calibration, Auto location, UTC+0, RA duty from board_config.h
Settings defaults();

// Replace NaN or out-of-range values by their defaults; returns how many were replaced
int sanitize(Settings& s);

// Image size of the current layout (header + payload); always <= STORE_SIZE
size_t encodedSize();
// Writes the image into out (at least encodedSize() bytes); returns its size
size_t encode(const Settings& s, uint8_t* out, size_t out_size);
// Reads an image; on anything but Loaded, s is set to defaults()
LoadResult decode(const uint8_t* in, size_t in_size, Settings& s);

uint32_t crc32(const uint8_t* data, size_t n);

// Short boot screen text for a load result, e.g. "6. Settings loaded"
const char* loadText(LoadResult r);

// Where the image lives (flash on the board, RAM on the PC)
class SettingsStore {
public:
  virtual ~SettingsStore() {}
  virtual bool read(uint8_t* buf, size_t n) = 0;
  virtual bool write(const uint8_t* buf, size_t n) = 0;
};

// Load from the store (defaults on any problem)
LoadResult load(SettingsStore& store, Settings& s);
// Save to the store; does not write when the stored image is already the same
SaveResult save(SettingsStore& store, const Settings& s);

// Store in memory (host tests and the UI simulator); starts erased like new flash
class RamSettingsStore : public SettingsStore {
public:
  RamSettingsStore() { erase(); }
  void erase() { for (size_t i = 0; i < STORE_SIZE; i++) data[i] = 0xFF; }
  bool read(uint8_t* buf, size_t n) override {
    if (n > STORE_SIZE) return false;
    for (size_t i = 0; i < n; i++) buf[i] = data[i];
    return true;
  }
  bool write(const uint8_t* buf, size_t n) override {
    if (n > STORE_SIZE || fail_writes) return false;
    for (size_t i = 0; i < n; i++) data[i] = buf[i];
    writes++;
    return true;
  }
  uint8_t data[STORE_SIZE];
  int  writes = 0;            // number of write() calls that changed the store
  bool fail_writes = false;   // test hook
};

} // namespace config

#endif // _CONFIG_SETTINGS_H_
